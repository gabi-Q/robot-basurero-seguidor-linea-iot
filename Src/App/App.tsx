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
  Filler,
  Title,
  TimeScale,
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
  LineElement,
  Filler,
  Title,
  TimeScale
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
  const [chartData, setChartData] = useState([]);

  const doughnutRef = useRef(null);
  const historyChartRef = useRef(null);

  const formatTimestamp = (timestamp) => {
    if (!timestamp) return '–';
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
  
 const getColorByLevel = (level) => {
    if (level <= 49) return '#10b981'; // verde
    if (level <= 70) return '#facc15'; // amarillo claro
    if (level <= 95) return '#f59e0b'; // amarillo oscuro
    return '#dc2626'; // rojo
  };

  useEffect(() => {
    const currentRef = db.ref('/sensor/currentStatus');
    const historyRef = db.ref('/sensor/historialLlenado');

    const fetchData = () => {
      currentRef.once('value').then((snapshot) => {
        const data = snapshot.val() || {};
        setFillLevel(data.porcentajeLlenado ?? 0);
        setDistance(data.distanciaResiduos_mm ?? 0);
        setLidStatus(data.tapaAbierta ? 'open' : 'closed');
        setPersonDetected(data.enMovimiento ? false : !!data.personaDetectada);
        setEnMovimiento(!!data.enMovimiento);
      });

      historyRef.once('value').then((snapshot) => {
        const data = snapshot.val();
        if (data) {
          const parsed = Object.keys(data).map((key) => {
            const ts = data[key].timestamp;
            return {
              id: key,
              level: data[key].porcentajeLlenado ?? 0,
              timestamp: typeof ts === 'number' && ts > 1000000000 ? ts : null,
            };
          }).filter((entry) => entry.timestamp !== null).sort((a, b) => a.timestamp - b.timestamp);

          setHistory(parsed.reverse());

          const now = Math.floor(Date.now() / 1000);
          const interval = 60; // 1 minuto
          const points = 10;

          const chartBins = [];
          for (let i = points - 1; i >= 0; i--) {
            const end = now - i * interval;
            const start = end - interval;
            const binItems = parsed.filter((h) => h.timestamp >= start && h.timestamp < end);
            const avg = binItems.length ? binItems.reduce((a, b) => a + b.level, 0) / binItems.length : 0;
            chartBins.push({
              timestamp: end,
              level: avg,
            });
          }

          setChartData(chartBins);
        }
      });
    };

    fetchData();
    const interval = setInterval(fetchData, 10000);
    return () => clearInterval(interval);
  }, []);

    useEffect(() => {
    if (doughnutRef.current) {
      const chartStatus = Chart.getChart(doughnutRef.current);
      if (chartStatus) chartStatus.destroy();

      new Chart(doughnutRef.current, {
        type: 'doughnut',
        data: {
          labels: ['Lleno (%)', 'Vacío (%)'],
          datasets: [
            {
              data: [fillLevel, 100 - fillLevel],
              backgroundColor: [getColorByLevel(fillLevel), '#d1d5db'],
              borderWidth: 1,
              cutout: '70%',
              rotation: -90,
              circumference: 180,
            },
          ],
        },
        options: {
          responsive: true,
          plugins: {
            legend: { display: false },
            tooltip: { enabled: false },
          },
        },
      });
    }
  }, [fillLevel]);


  useEffect(() => {
    if (historyChartRef.current && chartData.length > 0) {
      const chartStatus = Chart.getChart(historyChartRef.current);
      if (chartStatus) chartStatus.destroy();

      new Chart(historyChartRef.current, {
        type: 'line',
        data: {
          labels: chartData.map((d) => formatTimestamp(d.timestamp)),
          datasets: [
            {
              label: 'Nivel de llenado (cada minuto)',
              data: chartData.map((d) => d.level),
              borderColor: '#2563eb',
              backgroundColor: 'rgba(37, 99, 235, 0.1)',
              fill: true,
              tension: 0.3,
              pointRadius: 4,
              pointHoverRadius: 6,
              pointBackgroundColor: '#1d4ed8',
              pointBorderColor: '#fff',
              borderWidth: 2,
            },
          ],
        },
        options: {
          responsive: true,
          plugins: {
            title: {
              display: true,
              text: 'Historial de llenado del contenedor',
              font: { size: 20, weight: 'bold' },
              color: '#374151',
              padding: { top: 10, bottom: 20 },
            },
            tooltip: {
              mode: 'nearest',
              intersect: false,
              backgroundColor: '#1e40af',
              titleColor: '#fefce8',
              bodyColor: '#f1f5f9',
              borderColor: '#60a5fa',
              borderWidth: 1,
            },
             legend: {
              display: true,
              position: 'bottom',
              labels: {
                color: '#374151',
                font: { size: 12 },
              },
            },
          },
          scales: {
            y: {
              beginAtZero: true,
              max: 100,
              grid: { color: '#e5e7eb' },
              title: {
                display: true,
                text: 'Porcentaje (%)',
                font: { size: 14 },
                color: '#4b5563',
              },
              ticks: { color: '#4b5563' },
            },
            x: {
              grid: { display: false },
              ticks: {
                color: '#4b5563',
                maxRotation: 50,
                minRotation: 30,
                callback: function (value) {
                  return this.getLabelForValue(value).split(',')[1];
                },
              },
            },
          },
        },
      });
    }
  }, [chartData]);

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-slate-100 text-slate-800 font-inter overflow-hidden">
      <div className="fixed top-4 left-4 w-24 h-24 rounded-full border-4 border-sky-500 overflow-hidden shadow-lg ring-2 ring-white z-50">
        <img src="https://i.ibb.co/4gtSgCHj/avatar.png" alt="avatar" className="w-full h-full object-cover" />
      </div>
      <div className="text-center mt-2">
        <h1 className="text-3xl font-bold text-sky-700">Dashboard-Web Carrito Basurero Seguidor de Línea</h1>
        <p className="text-sm text-gray-600 italic">(Prototipo)</p>
      </div>
      <div className="max-w-6xl mx-auto grid grid-cols-3 gap-x-6 gap-y-8 p-6 pt-8 items-stretch">
        <div className="bg-white rounded-3xl shadow-lg p-4 text-center flex flex-col items-center justify-center">
          <h2 className="text-lg font-semibold text-gray-600 mb-2">Llenado / Distancia</h2>
          <div className="flex flex-col items-center justify-center -mt-2 mb-0">
            <canvas ref={doughnutRef} className="w-36 h-20 !mb-0 !mt-0" />
            <p
              className={`text-2xl font-extrabold mt-[-8px] ${
                fillLevel <= 49
                  ? 'text-green-600'
                  : fillLevel <= 70
                  ? 'text-yellow-500'
                  : fillLevel <= 95
                  ? 'text-yellow-600'
                  : 'text-red-600'
              }`}
            >
              {fillLevel.toFixed(2)}% {fillLevel >= 100 ? '⚠️' : ''}
            </p>
          </div>
        </div>
        <div className="flex flex-col justify-between gap-4 h-full">
          <div className="bg-white rounded-3xl shadow-lg p-4 flex-1 text-center flex flex-col justify-center">
            <h2 className="text-md font-medium text-gray-600">Tapa</h2>
            <p className="text-2xl font-semibold mt-3">
              {enMovimiento ? 'N/A' : (lidStatus === 'open' ? '🟢 Abierta' : '🔴 Cerrada')}
            </p>
          </div>
          <div className="bg-white rounded-3xl shadow-lg p-4 flex-1 text-center flex flex-col justify-center">
            <h2 className="text-md font-medium text-gray-600">Persona</h2>
            <p className="text-2xl font-semibold mt-3">
              {personDetected ? '👤 Detectada' : '🚫 No Detectada'}
            </p>
          </div>
        </div>

        <div className="bg-white rounded-3xl shadow-lg p-4 text-center flex items-center justify-center relative">
          <div>
            <h2 className="text-md font-medium text-gray-600">Carro Modo</h2>
            <p className="text-2xl font-semibold mt-3">
              {enMovimiento ? '🚛 Autonomo' : '🕒 Estacionario'}
            </p>
          </div>
        </div>
      </div>
      <div className="max-w-6xl mx-auto mt-4 px-6">
        <div className="bg-white rounded-3xl shadow-lg p-6">
          <div className="flex justify-between items-center mb-4">
            <h2 className="text-lg font-semibold text-indigo-600">Actividad Reciente</h2>
          </div>
          <div className="grid grid-cols-2 gap-4">
            <div className="h-64">
              <canvas ref={historyChartRef}></canvas>
            </div>
            <div className="overflow-auto max-h-64 text-sm">
              <table className="min-w-full border border-gray-300 shadow-sm rounded-lg overflow-hidden">
                <thead>
                  <tr className="bg-indigo-600 text-white text-sm uppercase">
                    <th className="px-4 py-2 text-left">📅 Fecha</th>
                    <th className="px-4 py-2 text-left">📊 Nivel (%)</th>
                  </tr>
                </thead>
                <tbody className="bg-white divide-y divide-gray-200">
                  {history.map((entry, index) => (
                    <tr key={entry.id} className={index % 2 === 0 ? 'bg-gray-50' : 'bg-white'}>
                      <td className="px-4 py-2 font-medium text-gray-800">{formatTimestamp(entry.timestamp)}</td>
                      <td className="px-4 py-2 text-blue-600 font-semibold">{entry.level.toFixed(2)}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default App;
            
