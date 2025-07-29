
import React from 'react';
import ReactDOM from 'react-dom/client';
import App from './App';

// Ensure Firebase and Chart.js are loaded before rendering App
// A more robust solution might involve a loading state or event listeners
// For simplicity, we assume they are available when App mounts.
const rootElement = document.getElementById('root');

if (!rootElement) {
  throw new Error("Could not find root element to mount to");
}

const root = ReactDOM.createRoot(rootElement);
root.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
