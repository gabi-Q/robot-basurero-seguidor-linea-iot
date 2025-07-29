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
  
