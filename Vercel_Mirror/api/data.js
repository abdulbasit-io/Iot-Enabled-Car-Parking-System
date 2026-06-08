const https = require('https');
const http = require('http');

function fetchUrl(url) {
  return new Promise((resolve, reject) => {
    const client = url.startsWith('https') ? https : http;
    client.get(url, (res) => {
      let data = '';
      res.on('data', (chunk) => { data += chunk; });
      res.on('end', () => {
        try {
          resolve(JSON.parse(data));
        } catch (e) {
          reject(new Error("Failed to parse JSON: " + data));
        }
      });
    }).on('error', (err) => {
      reject(err);
    });
  });
}

module.exports = async (req, res) => {
  // Set CORS headers for global access
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }

  const { type, predict } = req.query;
  const firebaseAuth = process.env.FIREBASE_AUTH || "RgVsCdCUiEB1Ian26wTMPmUMipVuktmWmDildvAW";
  const firebaseHost = "car-parking-system-a2064-default-rtdb.europe-west1.firebasedatabase.app";
  const predictionServiceUrl = process.env.PREDICTION_SERVICE_URL || "http://localhost:8000";

  try {
    if (predict) {
      const url = `${predictionServiceUrl}/predict?datetime=${encodeURIComponent(predict)}`;
      try {
        const predResult = await fetchUrl(url);
        return res.status(200).json(predResult);
      } catch (e) {
        // Fallback: If Render service is offline or sleeping, query Firebase predictions cache
        try {
          const predictionsUrl = `https://${firebaseHost}/parking/predictions.json?auth=${firebaseAuth}`;
          const predictionsData = await fetchUrl(predictionsUrl);
          
          if (predictionsData && predictionsData.forecast) {
            const reqDate = new Date(predict);
            const reqHour = String(reqDate.getHours()).padStart(2, '0');
            const reqMin = String(Math.floor(reqDate.getMinutes() / 15) * 15).padStart(2, '0');
            const reqTimeStr = `${reqHour}:${reqMin}`;
            
            // Find closest time in cache
            const match = predictionsData.forecast.find(f => f.time === reqTimeStr) || predictionsData.forecast[0];
            if (match) {
              return res.status(200).json({
                requested_time: predict,
                predicted_occupied_slots: Math.round(match.occupancy),
                predicted_occupancy_rate: match.occupancyRate,
                period_type: match.occupancy >= 7.5 ? "Peak" : (match.occupancy <= 2.0 ? "Very Low" : "Low/Normal"),
                source: "Firebase Cache Fallback"
              });
            }
          }
        } catch (cacheErr) {
          console.error("Cache fallback failed:", cacheErr);
        }
        return res.status(502).json({ error: "Prediction Service Offline", details: e.message });
      }
    }

    if (type === 'history') {
      const url = `https://${firebaseHost}/parking/history.json?auth=${firebaseAuth}&orderBy="$key"&limitToLast=30`;
      const historyData = await fetchUrl(url);
      return res.status(200).json(historyData || {});
    } else {
      const currentUrl = `https://${firebaseHost}/parking/current.json?auth=${firebaseAuth}`;
      const predictionsUrl = `https://${firebaseHost}/parking/predictions.json?auth=${firebaseAuth}`;
      
      const currentData = await fetchUrl(currentUrl);
      let predictionsData = {};
      
      try {
        predictionsData = await fetchUrl(predictionsUrl);
      } catch (predErr) {
        console.warn("Could not fetch predictions from Firebase:", predErr.message);
      }
      
      const merged = {
        ...currentData,
        predictions: predictionsData || {}
      };
      
      return res.status(200).json(merged);
    }
  } catch (error) {
    console.error("Firebase fetch error:", error);
    return res.status(500).json({ error: "Failed to fetch from Firebase", details: error.message });
  }
};
