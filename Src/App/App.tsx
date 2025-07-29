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
