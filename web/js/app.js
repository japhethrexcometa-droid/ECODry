(() => {
    'use strict';

    // Supabase Configuration
    const SUPABASE_URL = 'https://gzxmozyepmzcjzmzmbkx.supabase.co';
    const SUPABASE_ANON_KEY = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imd6eG1venllcG16Y2p6bXptYmt4Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODAzMDcyOTAsImV4cCI6MjA5NTg4MzI5MH0.X0MXPMptdNtE2Xrg9vZZ2i8n7kQtDtoukIXza3jyHlg';
    const supabase = window.supabase.createClient(SUPABASE_URL, SUPABASE_ANON_KEY);

    const REFRESH_INTERVAL = 5000;

    const DOM = {
        rainStatusValue: document.getElementById('rainStatusValue'),
        rainStatusSub: document.getElementById('rainStatusSub'),
        coverStatusValue: document.getElementById('coverStatusValue'),
        coverStatusSub: document.getElementById('coverStatusSub'),
        sensorValueValue: document.getElementById('sensorValueValue'),
        healthWater: document.getElementById('healthWater'),
        healthServo: document.getElementById('healthServo'),
        healthGSM: document.getElementById('healthGSM'),
        lastUpdated: document.getElementById('lastUpdated'),
        autoRefresh: document.getElementById('autoRefresh'),
        
        btnExportSensorCSV: document.getElementById('btnExportSensorCSV'),
        btnExportDBJSON: document.getElementById('btnExportDBJSON'),
        btnExportDBCSV: document.getElementById('btnExportDBCSV')
    };

    let sensorChart = null;
    let refreshTimer = null;

    function initChart() {
        const ctx = document.getElementById('sensorChart').getContext('2d');
        sensorChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'Sensor Value (Last 24h)',
                    data: [],
                    borderColor: '#3b82f6',
                    backgroundColor: 'rgba(59, 130, 246, 0.1)',
                    borderWidth: 2,
                    fill: true,
                    tension: 0.1,
                    pointRadius: 2,
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                plugins: {
                    legend: { display: false }
                },
                scales: {
                    x: {
                        grid: { color: '#f1f5f9' },
                        ticks: { font: { size: 10 } }
                    },
                    y: {
                        beginAtZero: true,
                        suggestedMax: 4095,
                        grid: { color: '#f1f5f9' }
                    }
                },
                animation: false
            }
        });
    }

    async function fetchData() {
        try {
            // Fetch latest 50 readings from last 24h from Supabase
            const twentyFourHoursAgo = new Date(Date.now() - 24 * 60 * 60 * 1000).toISOString();
            const { data: sensorData, error: sensorError } = await supabase
                .from('sensor_data')
                .select('*')
                .gte('created_at', twentyFourHoursAgo)
                .order('created_at', { ascending: false })
                .limit(50);
            
            if (sensorError) throw sensorError;

            if (sensorData && sensorData.length > 0) {
                const latest = sensorData[0]; // First is newest because of `order('created_at', false)`

                DOM.rainStatusValue.textContent = latest.rain_status === 'YES' ? 'RAINING' : 'CLEAR';
                DOM.rainStatusValue.style.color = latest.rain_status === 'YES' ? '#3b82f6' : '#10b981';
                DOM.rainStatusSub.textContent = latest.rain_status === 'YES' ? 'Rain detected' : 'No rain detected';
                
                DOM.coverStatusValue.textContent = latest.cover_status;
                DOM.coverStatusValue.style.color = latest.cover_status === 'OPEN' ? '#10b981' : '#f59e0b';
                DOM.coverStatusSub.textContent = latest.cover_status === 'OPEN' ? 'Rubber exposed' : 'Rubber protected';

                DOM.sensorValueValue.textContent = latest.sensor_value;

                // Update Chart
                if (sensorChart) {
                    const reversed = [...sensorData].reverse();
                    sensorChart.data.labels = reversed.map(d => {
                        const date = new Date(d.created_at);
                        return date.toLocaleTimeString([], {hour: '2-digit', minute:'2-digit'});
                    });
                    sensorChart.data.datasets[0].data = reversed.map(d => d.sensor_value);
                    sensorChart.update();
                }
            } else {
                DOM.rainStatusValue.textContent = "NO DATA";
                DOM.coverStatusValue.textContent = "NO DATA";
                DOM.sensorValueValue.textContent = "--";
            }

            // Fetch Health Data from Supabase
            const { data: healthData, error: healthError } = await supabase
                .from('system_health')
                .select('*')
                .limit(1)
                .single();

            if (healthData && !healthError) {
                DOM.healthWater.textContent = healthData.water_sensor;
                DOM.healthServo.textContent = healthData.servo;
                DOM.healthGSM.textContent = healthData.gsm;
            }

            const now = new Date();
            DOM.lastUpdated.textContent = `Last Updated: ${now.toLocaleTimeString()}`;

        } catch (e) {
            console.error("Error fetching data from Supabase:", e);
        }
    }

    // Export Functions
    function downloadFile(content, filename, contentType) {
        const a = document.createElement('a');
        const file = new Blob([content], { type: contentType });
        a.href = URL.createObjectURL(file);
        a.download = filename;
        a.click();
        URL.revokeObjectURL(a.href);
    }

    async function exportSensorCSV() {
        const { data } = await supabase.from('sensor_data').select('*').order('created_at', { ascending: true });
        if (!data || data.length === 0) return alert("No sensor data available.");
        
        let csv = "ID,Rain Status,Cover Status,Sensor Value,Timestamp\n";
        data.forEach(row => {
            csv += `${row.id},${row.rain_status},${row.cover_status},${row.sensor_value},${row.created_at}\n`;
        });
        downloadFile(csv, 'sensor_data.csv', 'text/csv');
    }

    async function exportDBJSON() {
        const { data: sensorData } = await supabase.from('sensor_data').select('*').order('created_at', { ascending: true });
        const { data: healthData } = await supabase.from('system_health').select('*');
        
        const fullDB = {
            sensor_data: sensorData || [],
            system_health: healthData || []
        };
        
        downloadFile(JSON.stringify(fullDB, null, 2), 'full_database.json', 'application/json');
    }

    async function exportDBCSV() {
        const { data: sensorData } = await supabase.from('sensor_data').select('*').order('created_at', { ascending: true });
        const { data: healthData } = await supabase.from('system_health').select('*').limit(1).single();
        
        let csv = "--- SYSTEM HEALTH ---\n";
        csv += "Water Sensor,Servo,SD Card,SD Space,GSM,Updated_At\n";
        if (healthData) {
            csv += `${healthData.water_sensor},${healthData.servo},${healthData.sd_card},${healthData.sd_space},${healthData.gsm},${healthData.updated_at}\n`;
        }
        
        csv += "\n--- SENSOR DATA ---\n";
        csv += "ID,Rain Status,Cover Status,Sensor Value,Timestamp\n";
        if (sensorData) {
            sensorData.forEach(row => {
                csv += `${row.id},${row.rain_status},${row.cover_status},${row.sensor_value},${row.created_at}\n`;
            });
        }
        downloadFile(csv, 'full_database.csv', 'text/csv');
    }

    function init() {
        // Bind Export Buttons
        DOM.btnExportSensorCSV.addEventListener('click', exportSensorCSV);
        DOM.btnExportDBJSON.addEventListener('click', exportDBJSON);
        DOM.btnExportDBCSV.addEventListener('click', exportDBCSV);

        initChart();
        fetchData();

        refreshTimer = setInterval(() => {
            if (DOM.autoRefresh.checked) {
                fetchData();
            }
        }, REFRESH_INTERVAL);
    }

    document.addEventListener('DOMContentLoaded', init);

})();
