// IMPORTANT: Replace with your actual Firebase project configuration!
// This configuration can be found in your Firebase project settings.
// Go to Project settings > General > Your apps > Web app > SDK setup and configuration

export const firebaseConfig = {
  apiKey: "YOUR_API_KEY", // Replace with your Firebase API key
  authDomain: "YOUR_PROJECT_ID.firebaseapp.com", // Replace with your Firebase auth domain
  databaseURL: "https://basurerointeligente-1d84d-default-rtdb.firebaseio.com/", // Replace with your Firebase Realtime Database URL
  projectId: "YOUR_PROJECT_ID", // Replace with your Firebase project ID
  storageBucket: "YOUR_PROJECT_ID.appspot.com", // Replace with your Firebase storage bucket
  messagingSenderId: "YOUR_MESSAGING_SENDER_ID", // Replace with your Firebase messaging sender ID
  appId: "YOUR_APP_ID", // Replace with your Firebase app ID
};

// Instructions for User:
// 1. Create a Firebase project at https://console.firebase.google.com/
// 2. In your Firebase project, go to "Project settings" (the gear icon).
// 3. Under the "General" tab, scroll down to "Your apps".
// 4. Click on the "Web" icon (</>) to add a web app (or select an existing one).
// 5. Register your app, and Firebase will provide you with the `firebaseConfig` object.
// 6. Copy these values and paste them into this file, replacing the placeholder strings.
// 7. Set up Realtime Database:
//    - In the Firebase console, go to "Build" > "Realtime Database".
//    - Click "Create Database".
//    - Choose a server location.
//    - Start in **Test mode** for initial development. This sets security rules to allow reads and writes:
//      {
//        "rules": {
//          ".read": "true",
//          ".write": "true"
//        }
//      }
//      **Important**: For a production app, secure your database with appropriate rules:
//      https://firebase.google.com/docs/database/security
//    - Your ESP32 should write data to a path like `smartBin`. Example structure:
//      {
//        "smartBin": {
//          "currentLevel": 0,            // Number (0-100)
//          "lastUpdate": 1678886400000,  // Firebase Server Timestamp
//          "history": {                  // Optional: for history log
//            "pushId1": { "level": 10, "timestamp": 1678886400000 },
//            "pushId2": { "level": 15, "timestamp": 1678886500000 }
//          }
//        }
//      }
//      When your ESP32 sends data, it should update `currentLevel`, `lastUpdate` (using `firebase.database.ServerValue.TIMESTAMP`),
//      and push new entries to `history` if you implement that feature.
