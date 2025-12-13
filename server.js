// これはイメージ用。実際は認証など足す想定。

const express = require('express');
const app = express();
app.use(express.json());

let latestEventIdByDevice = {}; // { deviceId: number }
let totalAmountByDevice = {};   // { deviceId: number }

app.post('/api/savings', (req, res) => {
const { amount, deviceId } = req.body;
if (!amount || !deviceId) {
return res.status(400).json({ error: 'amount and deviceId required' });
}

// 合計更新
totalAmountByDevice[deviceId] = (totalAmountByDevice[deviceId] || 0) + amount;
// イベントID更新
latestEventIdByDevice[deviceId] = (latestEventIdByDevice[deviceId] || 0) + 1;

return res.json({
ok: true,
latestEventId: latestEventIdByDevice[deviceId],
total: totalAmountByDevice[deviceId],
});
});

app.get('/api/latest-savings', (req, res) => {
const deviceId = req.query.deviceId;
if (!deviceId) return res.status(400).send('deviceId required');

const id = latestEventIdByDevice[deviceId] || 0;
// ESP32側を簡単にするため「数字だけ返す」仕様でもよい
res.type('text/plain').send(String(id));
});

app.listen(3000, () => console.log('server started on 3000'));