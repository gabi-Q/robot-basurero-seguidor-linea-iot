import React, { useState, useEffect, useRef } from 'react';
import firebase from 'firebase/compat/app'; // Firebase compatibility library
import 'firebase/compat/database'; // Firebase Realtime Database
import { Chart, DoughnutController, ArcElement, Tooltip, Legend, CategoryScale, LinearScale, LineController, PointElement, LineElement } from 'chart.js'; // Chart.js components
import { firebaseConfig } from './firebaseConfig';
import type { HistoryEntry, FirebaseHistoryEntry } from './types';
import { FILL_LEVEL_ALERT_THRESHOLD, FIREBASE_DATA_PATH, HISTORY_LOG_COUNT } from './constants';

// Register Chart.js components
Chart.register(DoughnutController, ArcElement, Tooltip, Legend, CategoryScale, LinearScale, LineController, PointElement, LineElement);

const App: React.FC = () => {
  // State for bin data
  const [currentLevel, setCurrentLevel] = useState<number | null>(null);
  const [lastUpdate, setLastUpdate] = useState<number | null>(null);
  const [distance, setDistance] = useState<number | null>(null);
  const [lidStatus, setLidStatus] = useState<boolean | null>(null); // true for open, false for closed
  const [personDetected, setPersonDetected] = useState<boolean>(false); // State for person detection
  const [history, setHistory] = useState<HistoryEntry[]>([]);
  const [error, setError] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState<boolean>(true);

  // Refs for the chart
  const chartRef = useRef<HTMLCanvasElement | null>(null);
  const chartInstanceRef = useRef<Chart<'doughnut'> | null>(null);
  const historyChartRef = useRef<HTMLCanvasElement | null>(null);
  const historyChartInstanceRef = useRef<Chart<'line'> | null>(null);

  // Initialize Firebase and set up data listeners
  useEffect(() => {
    try {
      if (!firebase.apps.length) {
        firebase.initializeApp(firebaseConfig);
      }
    } catch (err) {
      console.error("Firebase initialization error:", err);
      setError("Failed to initialize Firebase. Please check your configuration.");
      setIsLoading(false);
      return;
    }

    const db = firebase.database();
    const currentLevelRef = db.ref('/sensor/porcentaje');
    const lastUpdateRef = db.ref('/sensor/lastUpdate');
    const distanceRef = db.ref('/sensor/distancia_mm');
    const lidStatusRef = db.ref('/sensor/lidStatus'); // Assuming this path for lid status
    const personDetectedRef = db.ref('/sensor/personDetected'); // Assuming this path for person detection
    const historyRef = db.ref('/sensor/history').orderByChild('timestamp').limitToLast(HISTORY_LOG_COUNT);

    // Listener for current fill level
    const onCurrentLevelChange = currentLevelRef.on(
      'value',
      (snapshot) => {
        const level = snapshot.val();
        if (typeof level === 'number') {
          setCurrentLevel(Math.min(100, Math.max(0, level))); // Clamp between 0 and 100
        } else if (level === null && currentLevel !== null) {
          // Data might have been deleted, or path is incorrect
           setCurrentLevel(null); // Or set to a default like 0
        }
        setIsLoading(false);
      },
      (err) => {
        console.error("Error fetching current level:", err);
        setError("Could not fetch fill level data.");
        setIsLoading(false);
      }
    );

    // Listener for last update timestamp
    const onLastUpdateChange = lastUpdateRef.on(
      'value',
      (snapshot) => {
        const timestamp = snapshot.val();
        if (typeof timestamp === 'number') {
          setLastUpdate(timestamp);
        }
      },
      (err) => {
        console.error("Error fetching last update:", err);
        // Non-critical, so don't set a general error
      }
    );

    // Listener for distance data
    const onDistanceChange = distanceRef.on(
      'value',
      (snapshot) => {
        const dist = snapshot.val();
        if (typeof dist === 'number') {
          setDistance(dist);
        }
      },
      (err) => {
        console.error("Error fetching distance:", err);
      }
    );

    // Listener for lid status
    const onLidStatusChange = lidStatusRef.on(
      'value',
      (snapshot) => {
        const status = snapshot.val();
        if (typeof status === 'boolean') {
          setLidStatus(status);
        }
      },
      (err) => {
        console.error("Error fetching lid status:", err);
      }
    );

    // Listener for person detection status
    const onPersonDetectedChange = personDetectedRef.on(
      'value',
      (snapshot) => {
        const detected = snapshot.val();
        if (typeof detected === 'boolean') {
          setPersonDetected(detected);
        }
      },
      (err) => {
        console.error("Error fetching person detection status:", err);
      }
    );

    // Listener for history data
    const onHistoryChange = historyRef.on(
      'value',
      (snapshot) => {
        const data: Record<string, FirebaseHistoryEntry> = snapshot.val();
        if (data) {
          const formattedHistory: HistoryEntry[] = Object.keys(data)
            .map(key => ({
              id: key,
              level: data[key].level,
              timestamp: data[key].timestamp,
            }))
            .sort((a, b) => b.timestamp - a.timestamp); // Sort descending by timestamp
          setHistory(formattedHistory);
        } else {
          setHistory([]);
        }
      },
      (err) => {
        console.error("Error fetching history:", err);
        // Non-critical
      }
    );

    // Cleanup listeners on component unmount
    return () => {
      currentLevelRef.off('value', onCurrentLevelChange);
      lastUpdateRef.off('value', onLastUpdateChange);
      distanceRef.off('value', onDistanceChange);
      lidStatusRef.off('value', onLidStatusChange);
      personDetectedRef.off('value', onPersonDetectedChange);
      historyRef.off('value', onHistoryChange);
    };
  }, [currentLevel]); // Rerun if currentLevel was reset to null to re-establish listeners, though typically not needed.

  // Function to determine fill level color
  const getLevelColor = (level: number | null): string => {
    if (level === null) return '#e0e0e0'; // Default gray if no data
    if (level >= FILL_LEVEL_ALERT_THRESHOLD) return '#ef4444'; // Red (Tailwind red-500)
    if (level >= 70) return '#eab308'; // Yellow (Tailwind yellow-500)
    return '#22c55e'; // Green (Tailwind green-500)
  };

  // Update chart when currentLevel or chartRef changes
  useEffect(() => {
    if (chartRef.current && currentLevel !== null) {
      const ctx = chartRef.current.getContext('2d');
      if (!ctx) return;

      // Destroy previous chart instance if it exists
      if (chartInstanceRef.current) {
        chartInstanceRef.current.destroy();
      }

      const levelColor = getLevelColor(currentLevel);

      chartInstanceRef.current = new Chart(ctx, {
        type: 'doughnut',
        data: {
          labels: ['Filled', 'Empty'],
          datasets: [
            {
              data: [currentLevel, 100 - currentLevel],
              backgroundColor: [levelColor, '#e5e7eb'], // levelColor and Tailwind gray-200
              borderColor: ['#ffffff', '#ffffff'], // White border for separation
              borderWidth: 2,
              circumference: 180, // Half circle
              rotation: -90, // Start from the left
            },
          ],
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          cutout: '70%', // Adjust for thickness of the gauge
          plugins: {
            legend: {
              display: false, // No legend needed for a gauge
            },
            tooltip: {
              enabled: false, // No tooltip needed for a gauge
            },
          },
          animation: {
            animateRotate: true,
            animateScale: false,
          }
        },
      });
    } else if (chartInstanceRef.current && currentLevel === null) {
        chartInstanceRef.current.destroy();
        chartInstanceRef.current = null;
    }

    // Cleanup chart on component unmount or when currentLevel becomes null
    return () => {
      if (chartInstanceRef.current) {
        chartInstanceRef.current.destroy();
        chartInstanceRef.current = null;
      }
    };
  }, [currentLevel]); // Rerun when currentLevel changes

  // Effect for history chart
  useEffect(() => {
    if (historyChartRef.current && history.length > 0) {
      const ctx = historyChartRef.current.getContext('2d');
      if (!ctx) return;

      if (historyChartInstanceRef.current) {
        historyChartInstanceRef.current.destroy();
      }

      const sortedHistory = [...history].sort((a, b) => a.timestamp - b.timestamp);
      const labels = sortedHistory.map(entry => formatTimestamp(entry.timestamp));
      const data = sortedHistory.map(entry => entry.level);

      historyChartInstanceRef.current = new Chart(ctx, {
        type: 'line',
        data: {
          labels: labels,
          datasets: [
            {
              label: 'Nivel de Llenado (%)',
              data: data,
              borderColor: '#3b82f6', // Tailwind blue-500
              backgroundColor: 'rgba(59, 130, 246, 0.2)',
              fill: true,
              tension: 0.1,
            },
          ],
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          scales: {
            x: {
              type: 'category',
              title: {
                display: true,
                text: 'Fecha y Hora',
              },
            },
            y: {
              beginAtZero: true,
              max: 100,
              title: {
                display: true,
                text: 'Nivel de Llenado (%)',
              },
            },
          },
          plugins: {
            legend: {
              display: true,
            },
            tooltip: {
              mode: 'index',
              intersect: false,
            },
          },
        },
      });
    } else if (historyChartInstanceRef.current && history.length === 0) {
      historyChartInstanceRef.current.destroy();
      historyChartInstanceRef.current = null;
    }

    return () => {
      if (historyChartInstanceRef.current) {
        historyChartInstanceRef.current.destroy();
        historyChartInstanceRef.current = null;
      }
    };
  }, [history]);

  // Helper function to format timestamp
  const formatTimestamp = (timestamp: number): string => {
    if (!timestamp) return 'N/A';
    return new Date(timestamp).toLocaleString();
  };

  if (isLoading) {
    return (
      <div className="min-h-screen flex items-center justify-center bg-gray-100 p-4">
        <p className="text-xl text-gray-700">Loading Smart Bin Data...</p>
      </div>
    );
  }

  if (error) {
    return (
      <div className="min-h-screen flex flex-col items-center justify-center bg-red-100 p-4 text-center">
        <h1 className="text-3xl font-bold text-red-700 mb-4">Error</h1>
        <p className="text-red-600">{error}</p>
        <p className="mt-2 text-sm text-gray-600">Please ensure your Firebase configuration is correct and the database is accessible.</p>
      </div>
    );
  }

  const isAlertActive = currentLevel !== null && currentLevel >= FILL_LEVEL_ALERT_THRESHOLD;
  const levelColorClass =
    currentLevel === null ? 'text-gray-700' :
    isAlertActive ? 'text-red-600' :
    currentLevel >= 70 ? 'text-yellow-600' : 'text-green-600';


  return (
    <div className="min-h-screen bg-gray-100 flex flex-col items-center p-4 sm:p-6 lg:p-8 font-sans">
      <header className="mb-8 text-center">
        <h1 className="text-4xl font-bold text-gray-800">Dashboard de Contenedor Inteligente</h1>
        <p className="text-md text-gray-600">Monitoreo en tiempo real de tu basurero inteligente</p>
      </header>

      <main className="w-full max-w-5xl grid grid-cols-1 lg:grid-cols-3 gap-6">
        {/* Current Status Section */}
        <section className="lg:col-span-2 bg-white p-6 rounded-xl shadow-lg flex flex-col items-center">
          <h2 className="text-2xl font-semibold text-gray-700 mb-4">Nivel de Llenado Actual</h2>
          <div className="relative w-full max-w-xs h-48 sm:h-56 md:h-64 mb-4" role="img" aria-label={`Nivel de llenado: ${currentLevel ?? 'N/A'} por ciento`}>
            {currentLevel !== null ? (
              <canvas ref={chartRef}></canvas>
            ) : (
              <div className="absolute inset-0 flex items-center justify-center text-gray-500">Sin datos</div>
            )}
            {currentLevel !== null && (
              <div className="absolute inset-0 flex flex-col items-center justify-end pb-8 sm:pb-10">
                 <span className={`text-5xl font-bold ${levelColorClass}`} aria-live="polite">
                  {currentLevel}%
                </span>
              </div>
            )}
          </div>

          <p className="text-lg font-semibold text-gray-700 mt-4">
            Distancia: {distance !== null ? `${distance} mm` : 'N/A'}
          </p>

          {currentLevel !== null && isAlertActive && (
            <div role="alert" className="mt-4 p-3 bg-red-100 border-l-4 border-red-500 text-red-700 rounded-md w-full max-w-md text-center">
              <p className="font-bold">¡Alerta: Contenedor casi lleno!</p>
              <p>Nivel actual: {currentLevel}%. Por favor, vacíe pronto.</p>
            </div>
          )}

          <p className="text-sm text-gray-500 mt-6">
            Última actualización: <span className="font-medium">{lastUpdate ? formatTimestamp(lastUpdate) : 'N/A'}</span>
          </p>
        </section>

        {/* Lid Control Section */}
        <section className="lg:col-span-1 bg-white p-6 rounded-xl shadow-lg flex flex-col items-center justify-center">
          <h2 className="text-2xl font-semibold text-gray-700 mb-4">Control de Tapa</h2>
          <p className="text-xl font-bold mb-4">
            Estado de la Tapa: {lidStatus === true ? 'Abierta' : lidStatus === false ? 'Cerrada' : 'N/A'}
          </p>
          <button
            onClick={() => {
              const db = firebase.database();
              const lidControlRef = db.ref('/sensor/lidControl');
              lidControlRef.set(!lidStatus) // Toggle the lid status
                .then(() => console.log('Lid control updated successfully'))
                .catch((error) => console.error('Error updating lid control:', error));
            }}
            className="px-6 py-3 rounded-lg text-white font-semibold transition-colors duration-300
                       bg-blue-500 hover:bg-blue-600 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-opacity-50"
          >
            {lidStatus === true ? 'Cerrar Tapa' : 'Abrir Tapa'}
          </button>
        </section>

        {/* Person Detection Section */}
        <section className="lg:col-span-1 bg-white p-6 rounded-xl shadow-lg flex flex-col items-center justify-center">
          <h2 className="text-2xl font-semibold text-gray-700 mb-4">Detección de Persona</h2>
          <p className="text-xl font-bold mb-4">
            Estado: {personDetected ? 'Persona Detectada' : 'No Detectada'}
          </p>
          <div className={`w-24 h-24 rounded-full flex items-center justify-center text-white text-4xl font-bold
            ${personDetected ? 'bg-red-500' : 'bg-green-500'}`}>
            {personDetected ? '!' : 'OK'}
          </div>
        </section>

        {/* History Section */}
        <section className="lg:col-span-1 bg-white p-6 rounded-xl shadow-lg flex flex-col">
          <h2 className="text-2xl font-semibold text-gray-700 mb-4">Actividad Reciente</h2>
          {history.length > 0 ? (
            <div className="relative h-64 w-full">
              <canvas ref={historyChartRef}></canvas>
            </div>
          ) : (
            <p className="text-gray-500">No hay datos de historial disponibles.</p>
          )}
        </section>
      </main>

      <footer className="mt-12 text-center text-sm text-gray-500">
        <p>Data from ESP32 via Firebase Realtime Database.</p>
        <p>Powered by React, Chart.js, and Tailwind CSS.</p>
      </footer>
    </div>
  );
};

export default App;
