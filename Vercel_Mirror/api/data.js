const https = require('https');

function fetchUrl(url) {
  return new Promise((resolve, reject) => {
    https.get(url, (res) => {
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

  const { type } = req.query;
  const firebaseAuth = process.env.FIREBASE_AUTH || "RgVsCdCUiEB1Ian26wTMPmUMipVuktmWmDildvAW";
  const firebaseHost = "car-parking-system-a2064-default-rtdb.europe-west1.firebasedatabase.app";

  try {
    if (type === 'history') {
      const url = `https://${firebaseHost}/parking/history.json?auth=${firebaseAuth}&limitToLast=30`;
      const historyData = await fetchUrl(url);
      return res.status(200).json(historyData || {});
    } else {
      const url = `https://${firebaseHost}/parking/current.json?auth=${firebaseAuth}`;
      const currentData = await fetchUrl(url);
      return res.status(200).json(currentData || {});
    }
  } catch (error) {
    console.error("Firebase fetch error:", error);
    return res.status(500).json({ error: "Failed to fetch from Firebase", details: error.message });
  }
};
