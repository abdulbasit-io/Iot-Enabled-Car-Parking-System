#!/usr/bin/env python3
"""
IoT-Enabled Smart Car Parking System - ML Prediction Service
Department of Mechatronics Engineering, FUNAAB
"""

import os
import re
import json
import time
import urllib.request
import urllib.parse
import threading
from datetime import datetime, timedelta
import numpy as np
import pandas as pd
from sklearn.ensemble import HistGradientBoostingRegressor
from fastapi import FastAPI, Query
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI(title="Smart Parking Predictor API")

# Enable CORS for browser requests
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Configuration defaults (loaded from Environment Variables first)
db_host = os.getenv("FIREBASE_HOST", "car-parking-system-a2064-default-rtdb.europe-west1.firebasedatabase.app")
db_auth = os.getenv("FIREBASE_AUTH", "RgVsCdCUiEB1Ian26wTMPmUMipVuktmWmDildvAW")

# Fallback: Load credentials from Vercel config if available
if not os.getenv("FIREBASE_HOST") or not os.getenv("FIREBASE_AUTH"):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    data_js_path = os.path.join(script_dir, "..", "Vercel_Mirror", "api", "data.js")
    if os.path.exists(data_js_path):
        try:
            with open(data_js_path, "r", encoding="utf-8") as f:
                content = f.read()
                auth_match = re.search(r'firebaseAuth\s*=\s*process\.env\.FIREBASE_AUTH\s*\|\|\s*["\']([^"\']+)["\']', content)
                if auth_match and not os.getenv("FIREBASE_AUTH"):
                    db_auth = auth_match.group(1)
                host_match = re.search(r'firebaseHost\s*=\s*["\']([^"\']+)["\']', content)
                if host_match and not os.getenv("FIREBASE_HOST"):
                    db_host = host_match.group(1)
        except Exception as e:
            print(f"[*] Warning: Could not parse Vercel data.js: {e}")

# Global Model State: Non-linear Gradient Boosting Trees (no scaling needed!)
model = HistGradientBoostingRegressor(
    max_iter=100,
    learning_rate=0.1,
    max_depth=5,
    random_state=42
)
history_records = []
model_trained = False
lock = threading.Lock()

# Cooldown and State throttling (to avoid spamming calculations/writes on every heartbeat)
last_occupied_slots = None
last_forecast_time = 0.0
COOLDOWN_SECONDS = 600  # 10 minutes (600 seconds)

def get_cyclical_features(dt: datetime):
    """Encodes datetime into cyclical time, hourly one-hot, and weekend features."""
    hour_fraction = dt.hour + (dt.minute / 60.0)
    day = dt.weekday()
    
    # Cyclical representations
    hour_sin = np.sin(2 * np.pi * hour_fraction / 24.0)
    hour_cos = np.cos(2 * np.pi * hour_fraction / 24.0)
    day_sin = np.sin(2 * np.pi * day / 7.0)
    day_cos = np.cos(2 * np.pi * day / 7.0)
    
    # One-hot hour encoding (24 features)
    hour_indicators = [0.0] * 24
    hour_indicators[dt.hour] = 1.0
    
    # Weekend flag (1 feature)
    is_weekend = 1.0 if day >= 5 else 0.0
    
    # Total: 29 features
    return [hour_sin, hour_cos, day_sin, day_cos, is_weekend] + hour_indicators

def prepare_training_data(records):
    """Prepares 15-minute resampled feature vectors with lags and rolling averages (Options A & B)."""
    if len(records) < 15:
        return None, None
        
    # Build dataframe
    df = pd.DataFrame(records)
    df['dt'] = pd.to_datetime(df['timestamp'], unit='ms')
    df = df.sort_values('dt').set_index('dt')
    df = df[~df.index.duplicated(keep='last')]
    
    # Keep only the last 14 days of data to limit resampling grid size and focus on recent trends
    if not df.empty:
        max_dt = df.index.max()
        df = df[df.index >= (max_dt - pd.Timedelta(days=14))]
    
    # Resample to 15-minute grid & forward fill
    df_resampled = df['occupiedSlots'].resample('15min').ffill()
    df_resampled = pd.DataFrame(df_resampled)
    
    # Option A: Lags (15m, 30m, 1h ago)
    df_resampled['lag_15m'] = df_resampled['occupiedSlots'].shift(1)
    df_resampled['lag_30m'] = df_resampled['occupiedSlots'].shift(2)
    df_resampled['lag_1h'] = df_resampled['occupiedSlots'].shift(4)
    
    # Option B: Rolling Averages (last 30m, last 1h)
    df_resampled['roll_30m'] = df_resampled['occupiedSlots'].shift(1).rolling(window=2, min_periods=1).mean()
    df_resampled['roll_1h'] = df_resampled['occupiedSlots'].shift(1).rolling(window=4, min_periods=1).mean()
    
    # Drop rows containing NaNs
    df_resampled = df_resampled.dropna()
    
    if len(df_resampled) < 5:
        return None, None
        
    X = []
    y = []
    
    for idx, row in df_resampled.iterrows():
        # Get base 29 cyclical/time features
        time_feats = get_cyclical_features(idx)
        
        # Merge with Lags & Rolling features (34 total dimensions)
        all_feats = time_feats + [
            float(row['lag_15m']),
            float(row['lag_30m']),
            float(row['lag_1h']),
            float(row['roll_30m']),
            float(row['roll_1h'])
        ]
        X.append(all_feats)
        y.append(float(row['occupiedSlots']))
        
    return X, y

def train_model():
    """Fits the HistGradientBoostingRegressor model on historical records in memory."""
    global model, model_trained, history_records
    with lock:
        if len(history_records) < 15:
            print(f"[*] Not enough data points ({len(history_records)}) to train. Waiting for more history.")
            model_trained = False
            return False
            
        try:
            X, y = prepare_training_data(history_records)
            if X is not None and len(X) >= 5:
                model.fit(X, y)
                model_trained = True
                print(f"🤖 HistGradientBoostingRegressor model fitted successfully on {len(X)} data points.")
                return True
        except Exception as e:
            print(f"[x] Error during model training: {e}")
            
        return False

def load_history_and_train():
    """Downloads history from Firebase and runs initial training."""
    global history_records
    print("[*] Bootstrapping history data from Firebase...")
    url = f"https://{db_host}/parking/history.json?auth={db_auth}"
    
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as response:
            res_data = json.loads(response.read().decode('utf-8'))
            
            if res_data:
                records = []
                for k, v in res_data.items():
                    if isinstance(v, dict) and "timestamp" in v and "occupiedSlots" in v:
                        records.append(v)
                
                history_records = records
                print(f"[+] Loaded {len(history_records)} history records.")
                train_model()
            else:
                print("[!] History is empty. Awaiting new telemetry heartbeats.")
    except Exception as e:
        print(f"[x] Error downloading history: {e}")

def update_firebase_predictions():
    """Generates a 24-hour forecast recursively, calculates peak, and pushes to Firebase."""
    global model, model_trained, history_records
    if not model_trained:
        print("[*] Skipping Firebase forecast push: Model not trained yet.")
        return
        
    print("[*] Generating 24-hour occupancy forecast...")
    now = datetime.now()
    
    with lock:
        records = list(history_records)
        
    if len(records) < 15:
        return
        
    try:
        # Build seed DataFrame for recursive prediction lags
        df = pd.DataFrame(records)
        df['dt'] = pd.to_datetime(df['timestamp'], unit='ms')
        df = df.sort_values('dt').set_index('dt')
        df = df[~df.index.duplicated(keep='last')]
        
        # Keep only the last 14 days of data to limit resampling grid size
        if not df.empty:
            max_dt = df.index.max()
            df = df[df.index >= (max_dt - pd.Timedelta(days=14))]
        
        # Resample and fill
        df_resampled = df['occupiedSlots'].resample('15min').ffill()
        df_resampled = pd.DataFrame(df_resampled)
        
        # Take the last 8 time-steps for seeds
        df_seed = df_resampled.tail(8).copy()
        
        forecast_list = []
        peak_time = now
        peak_val = -1.0
        
        # Generate 24-hour forecast recursively
        for step in range(96):
            future_dt = now + timedelta(minutes=15 * step)
            
            # Extract inputs from seed data
            lag_15m = float(df_seed['occupiedSlots'].iloc[-1])
            lag_30m = float(df_seed['occupiedSlots'].iloc[-2]) if len(df_seed) >= 2 else lag_15m
            lag_1h = float(df_seed['occupiedSlots'].iloc[-4]) if len(df_seed) >= 4 else lag_30m
            
            roll_30m = float(df_seed['occupiedSlots'].iloc[-2:].mean()) if len(df_seed) >= 2 else lag_15m
            roll_1h = float(df_seed['occupiedSlots'].iloc[-4:].mean()) if len(df_seed) >= 4 else roll_30m
            
            time_feats = get_cyclical_features(future_dt)
            X_pred = [time_feats + [lag_15m, lag_30m, lag_1h, roll_30m, roll_1h]]
            
            # Predict & Clamp
            pred_val = float(model.predict(X_pred)[0])
            pred_val = max(0.0, min(10.0, pred_val))
            
            # Append prediction to seed DataFrame so it feeds the next prediction steps
            new_row = pd.DataFrame({'occupiedSlots': [pred_val]}, index=[pd.to_datetime(future_dt)])
            df_seed = pd.concat([df_seed, new_row])
            
            # Format object
            time_str = future_dt.strftime("%H:%M")
            forecast_list.append({
                "time": time_str,
                "iso": future_dt.isoformat(),
                "occupancy": round(pred_val, 1),
                "occupancyRate": round((pred_val / 10.0) * 100, 1)
            })
            
            if pred_val > peak_val:
                peak_val = pred_val
                peak_time = future_dt
                
        payload = {
            "nextPeakTime": peak_time.isoformat(),
            "nextPeakOccupancy": round(peak_val, 1),
            "nextPeakOccupancyRate": round((peak_val / 10.0) * 100, 1),
            "lastUpdated": int(time.time() * 1000),
            "forecast": forecast_list
        }
        
        # Write payload
        url = f"https://{db_host}/parking/predictions.json?auth={db_auth}"
        req_data = json.dumps(payload).encode('utf-8')
        req = urllib.request.Request(
            url, 
            data=req_data, 
            method='PUT',
            headers={'Content-Type': 'application/json', 'User-Agent': 'Mozilla/5.0'}
        )
        with urllib.request.urlopen(req) as response:
            print("✅ Successfully updated `/parking/predictions` node in Firebase.")
    except Exception as e:
        print(f"[x] Error during recursive forecasting: {e}")

def firebase_sse_listener():
    """Background listener subscribing to Firebase Realtime updates via Server-Sent Events."""
    global history_records
    url = f"https://{db_host}/parking/current.json?auth={db_auth}"
    
    while True:
        print("📡 Starting Firebase Realtime SSE listener connection...")
        try:
            req = urllib.request.Request(url, headers={
                'Accept': 'text/event-stream',
                'User-Agent': 'Mozilla/5.0'
            })
            
            with urllib.request.urlopen(req) as response:
                current_event = None
                
                # Read line-by-line
                for line in response:
                    line_str = line.decode('utf-8').strip()
                    if not line_str:
                        continue
                        
                    if line_str.startswith("event:"):
                        current_event = line_str.replace("event:", "").strip()
                    elif line_str.startswith("data:"):
                        data_val = line_str.replace("data:", "").strip()
                        if data_val == "null" or current_event != "put":
                            continue
                            
                        try:
                            event_data = json.loads(data_val)
                            path = event_data.get("path")
                            data_node = event_data.get("data")
                            
                            if path == "/" and isinstance(data_node, dict):
                                ts = data_node.get("timestamp")
                                occupied = data_node.get("occupiedSlots")
                                
                                if ts and occupied is not None:
                                    global last_occupied_slots, last_forecast_time
                                    
                                    now_time = time.time()
                                    slots_changed = (last_occupied_slots is None) or (occupied != last_occupied_slots)
                                    cooldown_passed = (now_time - last_forecast_time) >= COOLDOWN_SECONDS
                                    
                                    if slots_changed or cooldown_passed:
                                        print(f"📈 Processing Update: {occupied} slots occupied (slots_changed={slots_changed}, cooldown_passed={cooldown_passed})")
                                        new_rec = {"timestamp": ts, "occupiedSlots": occupied}
                                        history_records.append(new_rec)
                                        
                                        # Batch retraining
                                        train_model()
                                        
                                        last_occupied_slots = occupied
                                        last_forecast_time = now_time
                                        
                                        update_firebase_predictions()
                                    else:
                                        # Heartbeat received but skipped to save resources (no state change / inside cooldown)
                                        pass
                                        
                        except Exception as json_err:
                            print(f"[!] Error parsing event JSON: {json_err}")
                            
        except Exception as conn_err:
            print(f"[!] SSE Listener disconnected ({conn_err}). Reconnecting in 10 seconds...")
            time.sleep(10)

def scheduler_loop():
    """Background scheduler to update forecasts every 5 minutes."""
    while True:
        time.sleep(300)
        print("[*] Periodic scheduler: Refreshing model and predictions...")
        load_history_and_train()
        update_firebase_predictions()

@app.on_event("startup")
def startup_event():
    # Bootstrap data
    load_history_and_train()
    update_firebase_predictions()
    
    # Start background threads
    listener_thread = threading.Thread(target=firebase_sse_listener, daemon=True)
    listener_thread.start()
    
    scheduler_thread = threading.Thread(target=scheduler_loop, daemon=True)
    scheduler_thread.start()

@app.get("/")
def read_root():
    return {
        "status": "online",
        "service": "Smart Parking Prediction System",
        "model_trained": model_trained,
        "history_count": len(history_records)
    }

@app.get("/predict")
def predict_occupancy(
    dt_str: str = Query(
        None, 
        alias="datetime", 
        description="ISO Datetime string (e.g. 2026-06-08T18:30:00). Defaults to current time."
    )
):
    global model, model_trained, history_records
    
    try:
        if dt_str:
            dt = datetime.fromisoformat(dt_str)
        else:
            dt = datetime.now()
    except Exception:
        return {"error": "Invalid datetime format. Please use ISO 8601 (e.g. 2026-06-08T18:30:00)."}

    # Fallback if model not trained yet
    if not model_trained:
        if len(history_records) > 0:
            avg_occ = sum(r["occupiedSlots"] for r in history_records) / len(history_records)
        else:
            avg_occ = 0.0
        return {
            "requested_time": dt.isoformat(),
            "predicted_occupied_slots": round(avg_occ, 1),
            "predicted_occupancy_rate": round((avg_occ / 10.0) * 100, 1),
            "period_type": "Fallback (Insufficient training data)",
            "warning": "Model training cold-start. Showing historical baseline."
        }

    with lock:
        records = list(history_records)
        
    if len(records) < 15:
        return {"error": "Not enough history to compute features."}
        
    try:
        # Build features from current database state
        df = pd.DataFrame(records)
        df['dt'] = pd.to_datetime(df['timestamp'], unit='ms')
        df = df.sort_values('dt').set_index('dt')
        df = df[~df.index.duplicated(keep='last')]
        
        # Keep only the last 14 days of data to limit resampling grid size
        if not df.empty:
            max_dt = df.index.max()
            df = df[df.index >= (max_dt - pd.Timedelta(days=14))]
        df_resampled = df['occupiedSlots'].resample('15min').ffill()
        
        lag_15m = float(df_resampled.iloc[-1])
        lag_30m = float(df_resampled.iloc[-2]) if len(df_resampled) >= 2 else lag_15m
        lag_1h = float(df_resampled.iloc[-4]) if len(df_resampled) >= 4 else lag_30m
        
        roll_30m = float(df_resampled.iloc[-2:].mean()) if len(df_resampled) >= 2 else lag_15m
        roll_1h = float(df_resampled.iloc[-4:].mean()) if len(df_resampled) >= 4 else roll_30m
        
        time_feats = get_cyclical_features(dt)
        X_pred = [time_feats + [lag_15m, lag_30m, lag_1h, roll_30m, roll_1h]]
        
        with lock:
            pred_slots = model.predict(X_pred)[0]
            
        pred_slots = max(0.0, min(10.0, float(pred_slots)))
        
        period_type = "Low/Normal"
        if pred_slots >= 7.5:
            period_type = "Peak"
        elif pred_slots <= 2.0:
            period_type = "Very Low"
            
        return {
            "requested_time": dt.isoformat(),
            "predicted_occupied_slots": round(pred_slots, 1),
            "predicted_occupancy_rate": round((pred_slots / 10.0) * 100, 1),
            "period_type": period_type
        }
    except Exception as e:
        return {"error": f"Error computing prediction: {str(e)}"}

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
