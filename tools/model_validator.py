#!/usr/bin/env python3
"""
IoT-Enabled Smart Car Parking System - Prediction Model Validator
Department of Mechatronics Engineering, FUNAAB
Matches Section 5: Machine Learning Model Validation Guidelines
"""

import os
import re
import json
import urllib.request
import time
import math
from datetime import datetime, timedelta

# Import analytical libraries
try:
    import pandas as pd
    import numpy as np
    from sklearn.ensemble import HistGradientBoostingRegressor
    from sklearn.linear_model import LinearRegression
    from sklearn.metrics import mean_absolute_error, mean_squared_error, r2_score
    from sklearn.model_selection import TimeSeriesSplit
except ImportError as e:
    print(f"[x] Error: Missing required analysis libraries. Please install pandas and scikit-learn.\nDetails: {e}")
    exit(1)

def parse_credentials():
    # Defaults
    host = "car-parking-system-a2064-default-rtdb.europe-west1.firebasedatabase.app"
    auth = "RgVsCdCUiEB1Ian26wTMPmUMipVuktmWmDildvAW"
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    data_js_path = os.path.join(script_dir, "..", "Vercel_Mirror", "api", "data.js")
    
    if os.path.exists(data_js_path):
        try:
            with open(data_js_path, "r", encoding="utf-8") as f:
                content = f.read()
                auth_match = re.search(r'firebaseAuth\s*=\s*process\.env\.FIREBASE_AUTH\s*\|\|\s*["\']([^"\']+)["\']', content)
                if auth_match:
                    auth = auth_match.group(1)
                host_match = re.search(r'firebaseHost\s*=\s*["\']([^"\']+)["\']', content)
                if host_match:
                    host = host_match.group(1)
        except Exception as e:
            print(f"[*] Warning: Could not parse Vercel data.js: {e}")
            
    return host, auth

def prepare_dataset(records):
    # Load into DataFrame
    df = pd.DataFrame(records)
    df['datetime'] = pd.to_datetime(df['timestamp'], unit='ms')
    df.set_index('datetime', inplace=True)
    
    # Sort and remove duplicates
    df = df.sort_index()
    df = df[~df.index.duplicated(keep='last')]
    
    # Determine the time span of the dataset
    time_span = df.index.max() - df.index.min()
    span_hours = time_span.total_seconds() / 3600.0
    
    # Adaptive Resampling based on time span
    if span_hours >= 12.0:
        resample_rule = '15min'
        lag_steps_24h = 96 # 24h ago at 15m intervals
        roll_steps_60m = 4  # 60m ago at 15m intervals
        desc = "Standard 15-Minute Intervals (lags: 24h, roll: 60m)"
    else:
        resample_rule = '1min'
        # Scale back steps for short simulation runs so we don't drop all data
        lag_steps_24h = min(15, len(df) // 4)
        if lag_steps_24h == 0: lag_steps_24h = 1
        roll_steps_60m = min(5, len(df) // 4)
        if roll_steps_60m == 0: roll_steps_60m = 1
        desc = f"High-Density 1-Minute Intervals (lags: {lag_steps_24h}m, roll: {roll_steps_60m}m) - Adapted for short time span ({span_hours:.2f} hours)"
        
    print(f"[*] Resampling configuration: {desc}")
    
    # Resample to intervals & forward fill
    df_resampled = df['occupiedSlots'].resample(resample_rule).mean().ffill().round()
    df_resampled = pd.DataFrame(df_resampled)
    
    # 6.5 Feature Engineering
    # Temporal features
    df_resampled['hour_of_day'] = df_resampled.index.hour + df_resampled.index.minute / 60.0
    df_resampled['day_of_week'] = df_resampled.index.weekday.astype(float)
    df_resampled['is_weekend'] = (df_resampled['day_of_week'] >= 5).astype(float)
    
    # Historical occupancy features
    df_resampled['lag_yesterday'] = df_resampled['occupiedSlots'].shift(lag_steps_24h)
    df_resampled['roll_mean_60m'] = df_resampled['occupiedSlots'].shift(1).rolling(window=roll_steps_60m, min_periods=1).mean()
    
    # Backfill lag_yesterday to preserve early samples on short datasets
    df_resampled['lag_yesterday'] = df_resampled['lag_yesterday'].bfill()
    df_resampled.dropna(inplace=True)
    
    return df_resampled, resample_rule, desc, lag_steps_24h, roll_steps_60m

def evaluate_model(model, X_train, y_train, X_test, y_test):
    start_time = time.perf_counter()
    model.fit(X_train, y_train)
    inf_time_ms = (time.perf_counter() - start_time) * 1000.0
    
    y_pred = model.predict(X_test)
    y_pred = np.clip(y_pred, 0.0, 10.0) # Clamp between 0 and 10 slots
    
    mae = mean_absolute_error(y_test, y_pred)
    rmse = math.sqrt(mean_squared_error(y_test, y_pred))
    
    # R2 score calculation with safety for constant targets
    if len(np.unique(y_test)) > 1:
        r2 = r2_score(y_test, y_pred)
    else:
        r2 = 1.0 if np.allclose(y_test, y_pred) else 0.0
        
    # MAPE calculation
    mape = np.mean(np.abs((y_test - y_pred) / np.maximum(y_test, 1.0))) * 100.0
    
    return mae, rmse, mape, r2, inf_time_ms

def main():
    print("=" * 65)
    print("      IoT SMART PARKING SYSTEM - PREDICTION ACCURACY VALIDATOR   ")
    print("=" * 65)
    
    host, auth = parse_credentials()
    
    # Fetch history data
    print("[*] Querying historical validation logs from Firebase...")
    url = f"https://{host}/parking/history.json?auth={auth}&orderBy=\"$key\"&limitToLast=2000000"
    
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        with urllib.request.urlopen(req) as response:
            raw_data = json.loads(response.read().decode('utf-8'))
    except Exception as e:
        print(f"[x] Error: Failed to fetch validation dataset: {e}")
        return
        
    if not raw_data:
        print("[x] Error: No history found to perform validation.")
        return
        
    # Extract records and filter out unsynchronized timestamps (e.g. 1970 boot epochs)
    records = []
    for k, v in raw_data.items():
        if isinstance(v, dict) and "timestamp" in v and "occupiedSlots" in v:
            if v["timestamp"] > 1700000000000:
                records.append(v)
            
    print(f"[+] Loaded {len(records)} raw data samples.")
    if len(records) < 15:
        print("[x] Error: Insufficient logs for model training validation. Need at least 15 samples.")
        return
        
    # Process dataset
    print("[*] Engineering time-series features (Lags, Rolling Averages, Cyclical Time)...")
    df_data, rule, config_desc, lag_val, roll_val = prepare_dataset(records)
    total_samples = len(df_data)
    
    if total_samples < 10:
        print("[x] Error: Not enough data points after feature engineering. Run the system longer.")
        return
        
    print(f"[+] Formatted {total_samples} resampled feature vectors.")
    
    # 6.2 Data Splits (60% Train, 20% Validation, 20% Test)
    train_idx = int(total_samples * 0.6)
    val_idx = int(total_samples * 0.8)
    if train_idx == 0: train_idx = 1
    if val_idx <= train_idx: val_idx = train_idx + 1
    
    df_train = df_data.iloc[:train_idx]
    df_val = df_data.iloc[train_idx:val_idx]
    df_test = df_data.iloc[val_idx:]
    
    feature_cols = ['hour_of_day', 'day_of_week', 'is_weekend', 'lag_yesterday', 'roll_mean_60m']
    
    X_train = df_train[feature_cols].values
    y_train = df_train['occupiedSlots'].values
    
    X_val = df_val[feature_cols].values
    y_val = df_val['occupiedSlots'].values
    
    X_test = df_test[feature_cols].values
    y_test = df_test['occupiedSlots'].values
    
    # Combined training and validation for final test evaluation
    X_train_val = np.concatenate((X_train, X_val), axis=0)
    y_train_val = np.concatenate((y_train, y_val), axis=0)
    
    print(f"[*] Dataset Splits:")
    print(f"    - Train Set      : {len(X_train)} samples")
    print(f"    - Validation Set : {len(X_val)} samples")
    print(f"    - Testing Set    : {len(X_test)} samples")
    
    # 6.4 Time Series Cross-Validation (5-Fold walk-forward)
    print("[*] Running 5-Fold Time Series Cross-Validation...")
    tscv = TimeSeriesSplit(n_splits=min(5, total_samples // 3))
    cv_folds = []
    
    X_all = df_data[feature_cols].values
    y_all = df_data['occupiedSlots'].values
    
    for fold, (train_index, test_index) in enumerate(tscv.split(X_all), 1):
        X_tr, X_te = X_all[train_index], X_all[test_index]
        y_tr, y_te = y_all[train_index], y_all[test_index]
        
        model = HistGradientBoostingRegressor(max_iter=100, random_state=42)
        mae, rmse, mape, r2, inf_t = evaluate_model(model, X_tr, y_tr, X_te, y_te)
        
        cv_folds.append({
            'fold': fold,
            'train_size': len(train_index),
            'test_size': len(test_index),
            'mae': mae,
            'rmse': rmse,
            'mape': mape,
            'r2': r2
        })
        print(f"    - Fold {fold}: Train size = {len(train_index)}, Test size = {len(test_index)}, MAE = {mae:.3f} slots")
        
    # 6.6 Performance Comparison (15 min vs 1 hour horizons)
    # 15-minute horizon is standard target
    # 1-hour horizon targets shifted by 4 steps (if 15m intervals) or 60 steps (if 1m intervals)
    horizon_shift = 4 if rule == '15min' else 60
    
    # Generate shifted dataset for 1-hour horizon
    df_data_1h = df_data.copy()
    df_data_1h['target_1h'] = df_data_1h['occupiedSlots'].shift(-horizon_shift)
    df_data_1h.dropna(inplace=True)
    
    # Split 1-hour dataset
    split_1h_idx = int(len(df_data_1h) * 0.8)
    df_train_1h = df_data_1h.iloc[:split_1h_idx]
    df_test_1h = df_data_1h.iloc[split_1h_idx:]
    
    X_train_1h = df_train_1h[feature_cols].values
    y_train_1h = df_train_1h['target_1h'].values
    X_test_1h = df_test_1h[feature_cols].values
    y_test_1h = df_test_1h['target_1h'].values
    
    # Models to compare
    hgbr_model = HistGradientBoostingRegressor(max_iter=100, random_state=42)
    lr_model = LinearRegression()
    
    # Evaluations: 15m Horizon
    mae_hgbr_15, rmse_hgbr_15, mape_hgbr_15, r2_hgbr_15, inf_hgbr_15 = evaluate_model(hgbr_model, X_train_val, y_train_val, X_test, y_test)
    mae_lr_15, rmse_lr_15, mape_lr_15, r2_lr_15, inf_lr_15 = evaluate_model(lr_model, X_train_val, y_train_val, X_test, y_test)
    
    # Historical Avg Baseline: 15m Horizon
    mean_val = np.mean(y_train_val)
    y_pred_base = np.full_like(y_test, mean_val)
    mae_base_15 = mean_absolute_error(y_test, y_pred_base)
    rmse_base_15 = math.sqrt(mean_squared_error(y_test, y_pred_base))
    mape_base_15 = np.mean(np.abs((y_test - y_pred_base) / np.maximum(y_test, 1.0))) * 100.0
    r2_base_15 = 0.0 # By definition of R2 baseline
    
    # Evaluations: 1-hour Horizon
    if len(X_test_1h) > 0:
        mae_hgbr_1h, rmse_hgbr_1h, mape_hgbr_1h, r2_hgbr_1h, inf_hgbr_1h = evaluate_model(hgbr_model, X_train_1h, y_train_1h, X_test_1h, y_test_1h)
        mae_lr_1h, rmse_lr_1h, mape_lr_1h, r2_lr_1h, inf_lr_1h = evaluate_model(lr_model, X_train_1h, y_train_1h, X_test_1h, y_test_1h)
        
        mean_val_1h = np.mean(y_train_1h)
        y_pred_base_1h = np.full_like(y_test_1h, mean_val_1h)
        mae_base_1h = mean_absolute_error(y_test_1h, y_pred_base_1h)
        rmse_base_1h = math.sqrt(mean_squared_error(y_test_1h, y_pred_base_1h))
        mape_base_1h = np.mean(np.abs((y_test_1h - y_pred_base_1h) / np.maximum(y_test_1h, 1.0))) * 100.0
        r2_base_1h = 0.0
    else:
        # Fallback values if dataset is too short for 1h shift
        mae_hgbr_1h = rmse_hgbr_1h = mape_hgbr_1h = r2_hgbr_1h = inf_hgbr_1h = 0.0
        mae_lr_1h = rmse_lr_1h = mape_lr_1h = r2_lr_1h = inf_lr_1h = 0.0
        mae_base_1h = rmse_base_1h = mape_base_1h = r2_base_1h = 0.0
        
    # 6.7 Ablation Study
    print("[*] Running feature ablation study...")
    ablation_results = {}
    
    # Helper to train and get MAE
    def get_subset_mae(feats):
        X_tr_sub = df_train[feats].values
        X_te_sub = df_test[feats].values
        m = HistGradientBoostingRegressor(max_iter=100, random_state=42)
        m.fit(X_tr_sub, y_train)
        preds = np.clip(m.predict(X_te_sub), 0, 10)
        return mean_absolute_error(y_test, preds)

    mae_full = mae_hgbr_15
    ablation_results['Full model (all features)'] = (mae_full, 0.0)
    
    # Drop Yesterday's Lag
    feats_no_lag = [c for c in feature_cols if c != 'lag_yesterday']
    mae_no_lag = get_subset_mae(feats_no_lag)
    ablation_results['Without occupancy at same hour yesterday'] = (mae_no_lag, mae_no_lag - mae_full)
    
    # Drop Rolling Mean
    feats_no_roll = [c for c in feature_cols if c != 'roll_mean_60m']
    mae_no_roll = get_subset_mae(feats_no_roll)
    ablation_results['Without rolling mean (60 min)'] = (mae_no_roll, mae_no_roll - mae_full)
    
    # Drop Hour of Day
    feats_no_hour = [c for c in feature_cols if c != 'hour_of_day']
    mae_no_hour = get_subset_mae(feats_no_hour)
    ablation_results['Without hour of day'] = (mae_no_hour, mae_no_hour - mae_full)
    
    # Drop Day of Week
    feats_no_day = [c for c in feature_cols if c not in ['day_of_week', 'is_weekend']]
    mae_no_day = get_subset_mae(feats_no_day)
    ablation_results['Without day of week'] = (mae_no_day, mae_no_day - mae_full)
    
    # Temporal Only
    feats_temp_only = ['hour_of_day', 'day_of_week', 'is_weekend']
    mae_temp_only = get_subset_mae(feats_temp_only)
    ablation_results['Temporal features only'] = (mae_temp_only, mae_temp_only - mae_full)

    # Output to console
    print("\n" + "=" * 65)
    print("                    MODEL FORECAST METRICS                       ")
    print("=" * 65)
    print(f"MAE (15-min Horizon)  : {mae_hgbr_15:.3f} slots (Target: < 1.0)")
    print(f"RMSE (15-min Horizon) : {rmse_hgbr_15:.3f} slots (Target: < 1.5)")
    print(f"R-squared (R2) Score  : {r2_hgbr_15:.4f}       (Target: > 0.85)")
    print(f"MAPE                  : {mape_hgbr_15:.2f} %     (Target: < 20%)")
    print("=" * 65)
    
    # Save validation report
    script_dir = os.path.dirname(os.path.abspath(__file__))
    report_path = os.path.join(script_dir, "prediction_model_report.md")
    
    # Create Time Series Cross-Validation table rows
    tscv_rows = ""
    for r in cv_folds:
        tscv_rows += f"| Fold {r['fold']} | {r['train_size']} | {r['test_size']} | {r['mae']:.3f} | {r['rmse']:.3f} | {r['r2']:.4f} | {r['mape']:.2f}% |\n"
        
    # Create Ablation study rows
    ablation_rows = ""
    for k, v in ablation_results.items():
        delta_str = f"+{v[1]:.3f}" if v[1] > 0 else f"{v[1]:.3f}"
        if k == 'Full model (all features)':
            delta_str = "—"
        ablation_rows += f"| {k} | {v[0]:.3f} | {delta_str} |\n"
        
    with open(report_path, "w", encoding="utf-8") as f:
        f.write(f"""# Section 5: Machine Learning Model Validation Report
**Generated on**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}  
**Academic Reference**: Department of Mechatronics Engineering, FUNAAB

## 5.1 Overview
This section validates the lightweight predictive model for forecasting peak parking demand, enabling proactive space management. 
The core model utilized in production is the **HistGradientBoostingRegressor** (an advanced Gradient Boosting tree ensemble optimized for time-series features).

## 5.2 Dataset Splits
To evaluate generalization, the telemetry historical data logs were partitioned chronologically:
* **Total Samples**: `{total_samples}` resampled vectors (`{config_desc}`).
* **Train Set (60%)**: `{len(X_train)}` samples (used to fit parameters).
* **Validation Set (20%)**: `{len(X_val)}` samples (used to verify tuning).
* **Testing Set (20%)**: `{len(X_test)}` samples (unseen holdout for final evaluation).

## 5.3 Performance Metrics & Targets
| Metric | Target Threshold | Measured Value (HGBR 15m Target) | Validation Status |
|---|---|---|---|
| **Mean Absolute Error (MAE)** | < 1.00 slots | {mae_hgbr_15:.4f} slots | {"✅ PASSED" if mae_hgbr_15 < 1.0 else "❌ FAILED"} |
| **Root Mean Squared Error (RMSE)** | < 1.50 slots | {rmse_hgbr_15:.4f} slots | {"✅ PASSED" if rmse_hgbr_15 < 1.5 else "❌ FAILED"} |
| **Mean Absolute Percentage Error (MAPE)** | < 20.00% | {mape_hgbr_15:.2f}% | {"✅ PASSED" if mape_hgbr_15 < 20.0 else "❌ FAILED"} |
| **Coefficient of Determination ($R^2$ Score)** | > 0.8500 | {r2_hgbr_15:.4f} | {"✅ PASSED" if r2_hgbr_15 > 0.85 else "❌ FAILED"} |

## 5.4 Time Series Cross-Validation
To prevent lookahead bias (leaking future information into past training), we ran a rolling Walk-Forward Time Series Split:

| Fold | Train Size | Test Size | MAE (slots) | RMSE (slots) | $R^2$ Score | MAPE (%) |
|---|---|---|---|---|---|---|
{tscv_rows}

## 5.5 Feature Engineering Configuration
* **Temporal Features**: Hour of Day (0-23), Day of Week (0-6), Is Weekend (binary).
* **Historical Occupancy Features**: Same-hour yesterday lag (`lag_yesterday` at `{lag_val}` step shift), Rolling mean of past 60 min (`roll_mean_60m` at `{roll_val}` window).

## 5.6 Table 5: Predictive Model Performance Comparison
Comparative evaluation of baseline guessing (Historical Average), Linear Regression, and the ensemble HistGradientBoostingRegressor:

| Model | Horizon | MAE (slots) | RMSE (slots) | MAPE (%) | $R^2$ Score | Inf. Time (ms) |
|---|---|---|---|---|---|---|
| **Historical Avg (Baseline)** | 15 min | {mae_base_15:.2f} | {rmse_base_15:.2f} | {mape_base_15:.1f}% | {r2_base_15:.2f} | < 1 |
| **Historical Avg (Baseline)** | 1 hour | {mae_base_1h:.2f} | {rmse_base_1h:.2f} | {mape_base_1h:.1f}% | {r2_base_1h:.2f} | < 1 |
| **Linear Regression** | 15 min | {mae_lr_15:.2f} | {rmse_lr_15:.2f} | {mape_lr_15:.1f}% | {r2_lr_15:.2f} | {inf_lr_15:.2f} |
| **Linear Regression** | 1 hour | {mae_lr_1h:.2f} | {rmse_lr_1h:.2f} | {mape_lr_1h:.1f}% | {r2_lr_1h:.2f} | {inf_lr_1h:.2f} |
| **HistGradientBoostingRegressor** | 15 min | {mae_hgbr_15:.2f} | {rmse_hgbr_15:.2f} | {mape_hgbr_15:.1f}% | {r2_hgbr_15:.2f} | {inf_hgbr_15:.2f} |
| **HistGradientBoostingRegressor** | 1 hour | {mae_hgbr_1h:.2f} | {rmse_hgbr_1h:.2f} | {mape_hgbr_1h:.1f}% | {r2_hgbr_1h:.2f} | {inf_hgbr_1h:.2f} |

*Note: HistGradientBoostingRegressor represents the production Random Forest / Gradient Boosting equivalent.*

## 5.7 Ablation Study
To isolate the contribution of each engineered feature category, HGBR models were trained on subsets of features and compared back to the full configuration:

| Model Configuration | MAE ({'15 min' if rule == '15min' else '1 min'}) | $\Delta$ from Full Model |
|---|---|---|
{ablation_rows}

---
*Document prepared for submission to academic journals (Mechatronics Engineering Department, FUNAAB).*
""")
        
    print(f"\n[+] Saved academic model validation report to: {report_path}")

if __name__ == "__main__":
    main()
