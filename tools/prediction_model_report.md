# Section 5: Machine Learning Model Validation Report
**Generated on**: 2026-06-16 01:38:32  
**Academic Reference**: Department of Mechatronics Engineering, FUNAAB

## 5.1 Overview
This section validates the lightweight predictive model for forecasting peak parking demand, enabling proactive space management. 
The core model utilized in production is the **HistGradientBoostingRegressor** (an advanced Gradient Boosting tree ensemble optimized for time-series features).

## 5.2 Dataset Splits
To evaluate generalization, the telemetry historical data logs were partitioned chronologically:
* **Total Samples**: `745` resampled vectors (`Standard 15-Minute Intervals (lags: 24h, roll: 60m)`).
* **Train Set (60%)**: `447` samples (used to fit parameters).
* **Validation Set (20%)**: `149` samples (used to verify tuning).
* **Testing Set (20%)**: `149` samples (unseen holdout for final evaluation).

## 5.3 Performance Metrics & Targets
| Metric | Target Threshold | Measured Value (HGBR 15m Target) | Validation Status |
|---|---|---|---|
| **Mean Absolute Error (MAE)** | < 1.00 slots | 0.7186 slots | ✅ PASSED |
| **Root Mean Squared Error (RMSE)** | < 1.50 slots | 1.3266 slots | ✅ PASSED |
| **Mean Absolute Percentage Error (MAPE)** | < 20.00% | 17.82% | ✅ PASSED |
| **Coefficient of Determination ($R^2$ Score)** | > 0.8500 | 0.7708 | ❌ FAILED |

## 5.4 Time Series Cross-Validation
To prevent lookahead bias (leaking future information into past training), we ran a rolling Walk-Forward Time Series Split:

| Fold | Train Size | Test Size | MAE (slots) | RMSE (slots) | $R^2$ Score | MAPE (%) |
|---|---|---|---|---|---|---|
| Fold 1 | 125 | 124 | 1.953 | 3.413 | 0.0034 | 68.94% |
| Fold 2 | 249 | 124 | 1.690 | 2.097 | -53.7052 | 85.29% |
| Fold 3 | 373 | 124 | 0.378 | 0.528 | 0.0000 | 18.91% |
| Fold 4 | 497 | 124 | 0.068 | 0.119 | 0.0000 | 3.40% |
| Fold 5 | 621 | 124 | 0.801 | 1.412 | 0.7738 | 18.38% |


## 5.5 Feature Engineering Configuration
* **Temporal Features**: Hour of Day (0-23), Day of Week (0-6), Is Weekend (binary).
* **Historical Occupancy Features**: Same-hour yesterday lag (`lag_yesterday` at `96` step shift), Rolling mean of past 60 min (`roll_mean_60m` at `4` window).

## 5.6 Table 5: Predictive Model Performance Comparison
Comparative evaluation of baseline guessing (Historical Average), Linear Regression, and the ensemble HistGradientBoostingRegressor:

| Model | Horizon | MAE (slots) | RMSE (slots) | MAPE (%) | $R^2$ Score | Inf. Time (ms) |
|---|---|---|---|---|---|---|
| **Historical Avg (Baseline)** | 15 min | 1.53 | 3.16 | 24.1% | 0.00 | < 1 |
| **Historical Avg (Baseline)** | 1 hour | 1.52 | 3.16 | 23.8% | 0.00 | < 1 |
| **Linear Regression** | 15 min | 0.44 | 1.22 | 6.5% | 0.81 | 6.06 |
| **Linear Regression** | 1 hour | 0.89 | 2.13 | 12.4% | 0.41 | 3.72 |
| **HistGradientBoostingRegressor** | 15 min | 0.72 | 1.33 | 17.8% | 0.77 | 1274.24 |
| **HistGradientBoostingRegressor** | 1 hour | 1.63 | 2.65 | 54.9% | 0.09 | 943.89 |

*Note: HistGradientBoostingRegressor represents the production Random Forest / Gradient Boosting equivalent.*

## 5.7 Ablation Study
To isolate the contribution of each engineered feature category, HGBR models were trained on subsets of features and compared back to the full configuration:

| Model Configuration | MAE (15 min) | $\Delta$ from Full Model |
|---|---|---|
| Full model (all features) | 0.719 | — |
| Without occupancy at same hour yesterday | 0.943 | +0.225 |
| Without rolling mean (60 min) | 2.108 | +1.389 |
| Without hour of day | 0.645 | -0.074 |
| Without day of week | 0.637 | -0.082 |
| Temporal features only | 2.164 | +1.445 |


---
*Document prepared for submission to academic journals (Mechatronics Engineering Department, FUNAAB).*
