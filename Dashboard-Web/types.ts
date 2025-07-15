
// Represents the current data of the smart bin
export interface SmartBinData {
  currentLevel: number;
  lastUpdate: number; // Timestamp in milliseconds
}

// Represents a single entry in the history log
export interface HistoryEntry {
  id: string; // Firebase push ID
  level: number;
  timestamp: number; // Timestamp in milliseconds
}

// Structure of data in Firebase under /smartBin/history
export interface FirebaseHistoryEntry {
  level: number;
  timestamp: number;
}
