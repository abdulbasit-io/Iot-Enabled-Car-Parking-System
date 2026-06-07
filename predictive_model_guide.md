# Building a Self-Training Predictive Model for Parking Occupancy

To predict peak and low periods, your model needs to learn occupancy patterns based on **Time of Day** and **Day of the Week** (since parking demand is highly periodic). 

To make it **lightweight** and capable of **continuous training (online/incremental learning)** without needing to retrain on a massive dataset from scratch every time, you have two excellent approaches:

---

## Approach A: Binning with Exponential Moving Average (EMA)
*This is the most lightweight, robust, and easiest approach. It requires no heavy ML frameworks, runs in milliseconds, and can easily run on a cheap server, a Firebase Cloud Function, or even directly on the ESP32.*

### How it Works
1. Divide the week into **168 bins** (24 hours × 7 days).
2. For each bin, store a single value: the expected occupancy rate (0–100%).
3. When a new data point is uploaded from the ESP32, get the current **day of the week** and **hour**.
4. Update that specific bin's average using a running **Exponential Moving Average**:
   $$\text{New Average} = (1 - \alpha) \times \text{Old Average} + \alpha \times \text{Current Occupancy}$$
   *   Where $\alpha$ (alpha) is the **learning rate** (e.g., `0.05` or `5%`). A smaller alpha makes the model more stable, while a larger alpha lets it adapt to recent changes faster.

### Python Implementation (Subscribed to Firebase)
Here is a complete Python script that runs continuously, listens to Firebase changes, and updates its predictions on the fly:

```python
import time
import datetime
import firebase_admin
from firebase_admin import credentials, db

# 1. Initialize Firebase Admin SDK
cred = credentials.Certificate("serviceAccountKey.json") # Downloaded from Firebase
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://your-project-default-rtdb.firebaseio.com/'
})

# 2. Initialize the Model (168 bins: 7 days * 24 hours)
# In production, you would save/load this dictionary from Firebase or a local JSON file.
occupancy_model = {day: {hour: 0.0 for hour in range(24)} for day in range(7)}
learning_rate = 0.05 # How fast the model adapts to new data (alpha)

def update_model(event):
    """
    Callback function that triggers every time the ESP32 updates /parking/current
    """
    if event.data is None:
        return
        
    data = event.data
    current_occupancy = data.get("occupancyRate", 0)
    
    # Get the current local time of the event
    now = datetime.datetime.now()
    day = now.weekday() # 0 = Monday, 6 = Sunday
    hour = now.hour     # 0 to 23
    
    # Get previous prediction for this hour/day
    old_avg = occupancy_model[day][hour]
    
    # Continuous training: Update the average using EMA
    new_avg = ((1 - learning_rate) * old_avg) + (learning_rate * current_occupancy)
    occupancy_model[day][hour] = round(new_avg, 2)
    
    # Save the updated model back to Firebase under /parking/predictions
    db.reference("/parking/predictions").set(occupancy_model)
    
    print(f"📊 Live Update - Day: {day}, Hour: {hour} | Occ: {current_occupancy}% | Model Prediction: {occupancy_model[day][hour]}%")

# 3. Listen to live updates from the ESP32
db.reference("/parking/current").listen(update_model)

print("🚀 Listening for live parking data to update predictions...")
while True:
    time.sleep(1)
```

---

## Approach B: Incremental Machine Learning (Scikit-Learn `SGDRegressor`)
*Use this if you want a true machine learning model that can handle additional variables (like weather, local event calendars, or temperature) and find complex correlations.*

Standard machine learning models (like Random Forests or Deep Neural Networks) cannot easily update themselves with single new data points. However, linear models trained with **Stochastic Gradient Descent (SGD)** support **`partial_fit()`**, which enables continuous, single-sample learning.

### How it Works
1. Every time a new data point arrives, extract features:
   * **Hour of day** (encoded as $\sin$ and $\cos$ values so `23:00` is recognized as close to `00:00`).
   * **Day of week** (encoded as $\sin$ and $\cos$).
   * **Power usage** or other telemetry.
2. Call `model.partial_fit(X, y)` to slightly adjust the weights of the model.
3. Save the model weights (e.g., using `pickle` or `joblib`) so it doesn't lose progress if restarted.

### Python Implementation
```python
import numpy as np
import pickle
import os
from sklearn.linear_model import SGDRegressor

# Load existing model or create a new one
model_file = "parking_sgd_model.pkl"
if os.path.exists(model_file):
    with open(model_file, 'rb') as f:
        model = pickle.load(f)
else:
    model = SGDRegressor(learning_rate='constant', eta0=0.01)

def get_time_features(dt):
    """
    Encode cyclical time features (hour & day) using sine/cosine representation.
    """
    hour = dt.hour
    day = dt.weekday()
    
    # Cyclical encoding
    hour_sin = np.sin(2 * np.pi * hour / 24.0)
    hour_cos = np.cos(2 * np.pi * hour / 24.0)
    day_sin = np.sin(2 * np.pi * day / 7.0)
    day_cos = np.cos(2 * np.pi * day / 7.0)
    
    return np.array([[hour_sin, hour_cos, day_sin, day_cos]])

def train_one_step(current_occupancy, date_time):
    # 1. Prepare Features & Target
    X = get_time_features(date_time)
    y = np.array([current_occupancy])
    
    # 2. Incremental online training step
    model.partial_fit(X, y)
    
    # 3. Save the updated model
    with open(model_file, 'wb') as f:
        pickle.dump(model, f)

def predict_future_occupancy(target_datetime):
    X = get_time_features(target_datetime)
    predicted_value = model.predict(X)[0]
    return max(0.0, min(100.0, predicted_value)) # Clamp output between 0% and 100%
```

---

## Which one should you choose?

*   **Choose Approach A (Binning + EMA)** if you want something simple, bulletproof, and easy to interpret. You can look directly at the `/parking/predictions` node in Firebase to see a weekly calendar of expected occupancy rates, which makes drawing a "Peak/Low hours" chart in your web dashboard extremely straightforward.
*   **Choose Approach B (SGDRegressor)** if you want to experiment with true machine learning, or if you plan to introduce other variables (like holiday schedules or rain forecast API data) to improve prediction accuracy.
