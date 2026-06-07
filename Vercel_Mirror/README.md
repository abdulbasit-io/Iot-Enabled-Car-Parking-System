# Vercel Mirror Dashboard

This is a remote mirror web application of the Smart Parking system designed to run on Vercel's edge infrastructure. It pulls live slot and power telemetry from the Firebase Realtime Database.

---

## 1. Features
*   **Remote Web Viewing**: Check parking availability globally without connecting to the local Wi-Fi.
*   **Stale Data Detection**: Dynamically calculates the delay between the client browser time and the last ESP32 database write. If it exceeds 120 seconds, it displays an offline banner (**"⚠️ Live Connection Stale"**).
*   **Secure API Proxying**: Employs a Vercel Serverless Function proxy at `/api/data` to fetch data from Firebase securely, hiding your database secrets from public clients.
*   **Interactive History Logs**: The Admin Dashboard queries `/parking/history` to load the last 30 database events into a terminal console log.

---

## 2. Setup & Deployment

You can test and deploy this folder to Vercel easily.

### A. Environment Variables
To ensure the backend serverless proxy function is secure, add the following key-value pair in your Vercel Project Environment Variables:

| Variable Name | Description | Value |
|---|---|---|
| `FIREBASE_AUTH` | Legacy database secret token for authentication | `RgVsCdCUiEB1Ian26wTMPmUMipVuktmWmDildvAW` |

*(If you don't configure this, the code will default to the hardcoded database secret as a fallback).*

### B. Deployment Commands

1.  **Install Vercel CLI**:
    ```bash
    npm install -g vercel
    ```

2.  **Test Locally**:
    Run a local server mimicking Vercel's environment (including serverless functions):
    ```bash
    vercel dev
    ```

3.  **Deploy to Vercel**:
    Deploy the mirror folder to the cloud:
    ```bash
    vercel
    ```
    Follow the prompts in the terminal to complete your deployment.

4.  **Promote to Production**:
    Once happy with the deployment preview, promote it to your live domain:
    ```bash
    vercel --prod
    ```
