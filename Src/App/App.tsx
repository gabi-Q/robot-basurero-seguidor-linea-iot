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