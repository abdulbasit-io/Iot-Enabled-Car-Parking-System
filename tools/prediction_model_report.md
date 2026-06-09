# Section 5: Machine Learning Model Validation Report
**Generated on**: 2026-06-09 12:27:38  
**Academic Reference**: Department of Mechatronics Engineering, FUNAAB

## 5.1 Overview
This section validates the lightweight predictive model for forecasting peak parking demand, enabling proactive space management. 
The core model utilized in production is the **HistGradientBoostingRegressor** (an advanced Gradient Boosting tree ensemble optimized for time-series features).

## 5.2 Dataset Splits
To evaluate generalization, the telemetry historical data logs were partitioned chronologically:
* **Total Samples**: `152` resampled vectors (`Standard 15-Minute Intervals (lags: 24h, roll: 60m)`).
* **Train Set (60%)**: `91` samples (used to fit parameters).
* **Validation Set (20%)**: `30` samples (used to verify tuning).
* **Testing Set (20%)**: `31` samples (unseen holdout for final evaluation).

## 5.3 Performance Metrics & Targets
| Metric | Target Threshold | Measured Value (HGBR 15m Target) | Validation Status |
|---|---|---|---|
| **Mean Absolute Error (MAE)** | < 1.00 slots | 3.5771 slots | ❌ FAILED |
| **Root Mean Squared Error (RMSE)** | < 1.50 slots | 5.2568 slots | ❌ FAILED |
| **Mean Absolute Percentage Error (MAPE)** | < 20.00% | 45.42% | ❌ FAILED |
| **Coefficient of Determination ($R^2$ Score)** | > 0.8500 | -0.3423 | ❌ FAILED |

## 5.4 Time Series Cross-Validation
To prevent lookahead bias (leaking future information into past training), we ran a rolling Walk-Forward Time Series Split:

| Fold | Train Size | Test Size | MAE (slots) | RMSE (slots) | $R^2$ Score | MAPE (%) |
|---|---|---|---|---|---|---|
| Fold 1 | 27 | 25 | 0.148 | 0.148 | 0.0000 | 14.81% |
| Fold 2 | 52 | 25 | 1.192 | 1.587 | -0.6579 | 56.87% |
| Fold 3 | 77 | 25 | 2.714 | 3.926 | -0.6606 | 59.93% |
| Fold 4 | 102 | 25 | 0.298 | 0.873 | 0.0000 | 29.85% |
| Fold 5 | 127 | 25 | 4.385 | 5.781 | -0.6402 | 55.97% |


## 5.5 Feature Engineering Configuration
* **Temporal Features**: Hour of Day (0-23), Day of Week (0-6), Is Weekend (binary).
* **Historical Occupancy Features**: Same-hour yesterday lag (`lag_yesterday` at `96` step shift), Rolling mean of past 60 min (`roll_mean_60m` at `4` window).

## 5.6 Table 5: Predictive Model Performance Comparison
Comparative evaluation of baseline guessing (Historical Average), Linear Regression, and the ensemble HistGradientBoostingRegressor:

| Model | Horizon | MAE (slots) | RMSE (slots) | MAPE (%) | $R^2$ Score | Inf. Time (ms) |
|---|---|---|---|---|---|---|
| **Historical Avg (Baseline)** | 15 min | 4.13 | 5.49 | 93.8% | 0.00 | < 1 |
| **Historical Avg (Baseline)** | 1 hour | 4.23 | 5.58 | 93.1% | 0.00 | < 1 |
| **Linear Regression** | 15 min | 1.35 | 2.23 | 24.2% | 0.76 | 4.09 |
| **Linear Regression** | 1 hour | 3.59 | 5.01 | 62.3% | -0.21 | 3.54 |
| **HistGradientBoostingRegressor** | 15 min | 3.58 | 5.26 | 45.4% | -0.34 | 479.95 |
| **HistGradientBoostingRegressor** | 1 hour | 3.82 | 5.56 | 48.2% | -0.49 | 258.87 |

*Note: HistGradientBoostingRegressor represents the production Random Forest / Gradient Boosting equivalent.*

## 5.7 Ablation Study
To isolate the contribution of each engineered feature category, HGBR models were trained on subsets of features and compared back to the full configuration:

| Model Configuration | MAE (15 min) | $\Delta$ from Full Model |
|---|---|---|
| Full model (all features) | 3.577 | — |
| Without occupancy at same hour yesterday | 3.311 | -0.266 |
| Without rolling mean (60 min) | 4.151 | +0.574 |
| Without hour of day | 3.154 | -0.423 |
| Without day of week | 3.311 | -0.266 |
| Temporal features only | 4.151 | +0.574 |


---
*Document prepared for submission to academic journals (Mechatronics Engineering Department, FUNAAB).*
