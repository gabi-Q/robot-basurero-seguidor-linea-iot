import React, { useState, useEffect, useRef } from 'react';
import firebase from 'firebase/compat/app';
import 'firebase/compat/database';
import {
  Chart,
  DoughnutController,
  ArcElement,
  Tooltip,
  Legend,
  CategoryScale,
  LinearScale,
  LineController,
  PointElement,
  LineElement,
} from 'chart.js';
import { firebaseConfig } from './firebaseConfig';

Chart.register(
  DoughnutController,
  ArcElement,
  Tooltip,
  Legend,
  CategoryScale,
  LinearScale,
  LineController,
  PointElement,
  LineElement
);

firebase.initializeApp(firebaseConfig);
const db = firebase.database();

const App = () => {
  const [fillLevel, setFillLevel] = useState(0);
  const [distance, setDistance] = useState(0);
  const [lidStatus, setLidStatus] = useState('');
  const [personDetected, setPersonDetected] = useState(false);
  const [enMovimiento, setEnMovimiento] = useState(false);
  const [history, setHistory] = useState([]);

  const doughnutRef = useRef(null);
  const historyChartRef = useRef(null);

  const formatTimestamp = (timestamp) => {
    const date = new Date(timestamp * 1000);
    return date.toLocaleString('es-PE', {
      day: '2-digit',
      month: '2-digit',
      year: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
      hour12: false,
    });
  };

  useEffect(() => {
    const currentRef = db.ref('/sensor/currentStatus');
    const historyRef = db.ref('/sensor/history');

    currentRef.on('value', (snapshot) => {
      const data = snapshot.val() || {};
      setFillLevel(data.porcentajeLlenado ?? 0);
      setDistance(data.distanciaResiduos_mm ?? 0);
      setLidStatus(data.tapaAbierta ? 'open' : 'closed');
      setPersonDetected(!!data.personaDetectada);
      setEnMovimiento(!!data.enMovimiento);
    });

    historyRef.on('value', (snapshot) => {
      const data = snapshot.val();
      if (data) {
        const parsedHistory = Object.keys(data)
          .map((key) => ({
            id: key,
            level: data[key].level ?? 0,
            distance_mm: data[key].distance_mm ?? 0,
            timestamp: data[key].timestamp ?? 0,
          }))
          .sort((a, b) => b.timestamp - a.timestamp);
        setHistory(parsedHistory.slice(0, 10));
      }
    });

    return () => {
      currentRef.off();
      historyRef.off();
    };
  }, []);

  useEffect(() => {
    if (doughnutRef.current) {
      const chartStatus = Chart.getChart(doughnutRef.current);
      if (chartStatus) {
        chartStatus.destroy();
      }

      new Chart(doughnutRef.current, {
        type: 'doughnut',
        data: {
          labels: ['Lleno (%)', 'Vacío (%)'],
          datasets: [
            {
              data: [fillLevel, 100 - fillLevel],
              backgroundColor: ['#4ade80', '#e5e7eb'],
              borderWidth: 1,
            },
          ],
        },
        options: {
          responsive: true,
          plugins: {
            legend: { position: 'bottom' },
          },
        },
      });
    }
  }, [fillLevel]);

  useEffect(() => {
    if (historyChartRef.current && history.length > 0) {
      const chartStatus = Chart.getChart(historyChartRef.current);
      if (chartStatus) {
        chartStatus.destroy();
      }

      new Chart(historyChartRef.current, {
        type: 'line',
        data: {
          labels: history.map((h) => formatTimestamp(h.timestamp)).reverse(),
          datasets: [
            {
              label: 'Nivel (%)',
              data: history.map((h) => h.level).reverse(),
              borderColor: '#3b82f6',
              backgroundColor: 'rgba(59,130,246,0.2)',
              tension: 0.3,
              fill: true,
            },
          ],
        },
        options: {
          responsive: true,
          scales: {
            y: {
              beginAtZero: true,
              max: 100,
            },
          },
        },
      });
    }
  }, [history]);

  return (
    <div className="min-h-screen bg-gray-100 p-6 font-sans text-gray-800">
      <div className="max-w-5xl mx-auto grid grid-cols-1 md:grid-cols-2 gap-6">
        <div className="bg-white rounded-2xl shadow p-4">
          <h2 className="text-xl font-semibold mb-2">Nivel de Llenado</h2>
          <canvas ref={doughnutRef} className="max-h-60 mx-auto" />
          <p className="text-center mt-2 text-lg">
            {fillLevel.toFixed(2)}%
          </p>
        </div>

        <div className="bg-white rounded-2xl shadow p-4">
          <h2 className="text-xl font-semibold mb-2">Distancia Actual</h2>
          <p className="text-3xl font-bold text-center">{distance} mm</p>
        </div>

        <div className="bg-white rounded-2xl shadow p-4">
          <h2 className="text-xl font-semibold mb-2">Estado de la Tapa</h2>
          <p className="text-center text-2xl">
            {lidStatus === 'open' ? '🟢 Abierta' : '🔴 Cerrada'}
          </p>
        </div>

        <div className="bg-white rounded-2xl shadow p-4">
          <h2 className="text-xl font-semibold mb-2">Persona Detectada</h2>
          <p className="text-center text-2xl">
            {personDetected ? '👤 Sí' : '🚫 No'}
          </p>
        </div>

        <div className="bg-white rounded-2xl shadow p-4">
          <h2 className="text-xl font-semibold mb-2">Estado del Carro</h2>
          <p className="text-center text-2xl">
            {enMovimiento ? '🚛 Camino a vaciarse' : '🕒 Esperando basura'}
          </p>
        </div>

        <div className="bg-white rounded-2xl shadow p-4 md:col-span-2">
          <h2 className="text-xl font-semibold mb-2">Actividad Reciente</h2>
          {history.length > 0 ? (
            <>
              <div className="relative h-64 w-full mb-4">
                <canvas ref={historyChartRef}></canvas>
              </div>
              <div className="overflow-auto max-h-48 text-sm">
                <table className="min-w-full text-left border border-gray-300 rounded">
                  <thead className="bg-gray-200">
                    <tr>
                      <th className="px-2 py-1 border-b">Fecha</th>
                      <th className="px-2 py-1 border-b">Nivel (%)</th>
                      <th className="px-2 py-1 border-b">Distancia (mm)</th>
                    </tr>
                  </thead>
                  <tbody>
                    {history.map((entry) => (
                      <tr key={entry.id} className="border-b">
                        <td className="px-2 py-1">{formatTimestamp(entry.timestamp)}</td>
                        <td className="px-2 py-1">{entry.level.toFixed(2)}</td>
                        <td className="px-2 py-1">{entry.distance_mm}</td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            </>
          ) : (
            <p className="text-gray-500">No hay datos de historial disponibles.</p>
          )}
        </div>
      </div>
    </div>
  );
};

export default App;
