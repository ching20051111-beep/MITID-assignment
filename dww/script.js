// Web Serial protocol preserved from the working Arduino firmware.
const $ = id => document.getElementById(id);
let port, reader, writer, reading = false,
    buffer = '',
    rows = [];

function show(id) {
    for (const x of ['intro', 'test', 'results']) $(x).hidden = x !== id;
}

function msg(s) {
    $('message').textContent = s;
}

function lights(name) {
    document.querySelectorAll('[data-name]').forEach(x => x.classList.toggle('active', x.dataset.name === name));
}
async function send(s) {
    if (!writer) throw Error('Not connected');
    await writer.write(new TextEncoder().encode(s + '\n'));
}
async function connect() {
    if (!('serial' in navigator)) {
        msg('Web Serial requires desktop Chrome/Edge on localhost.');
        return;
    }
    try {
        port = await navigator.serial.requestPort();
        await port.open({
            baudRate: 115200
        });
        writer = port.writable.getWriter();
        reader = port.readable.getReader();
        reading = true;
        $('start').disabled = false;
        $('connection').textContent = 'Connected to Arduino UNO';
        msg('Connected. Press Start.');
        readLoop();
    } catch (e) {
        msg('Connection failed: ' + e.message);
    }
}
async function readLoop() {
    try {
        while (reading) {
            const {
                value,
                done
            } = await reader.read();
            if (done) break;
            buffer += new TextDecoder().decode(value);
            let i;
            while ((i = buffer.indexOf('\n')) >= 0) {
                let line = buffer.slice(0, i).trim();
                buffer = buffer.slice(i + 1);
                if (line) handle(line);
            }
        }
    } catch (e) {
        msg('Serial connection lost: ' + e.message);
    } finally {
        reading = false;
        $('start').disabled = true;
        $('connection').textContent = 'Disconnected';
        lights('');
        try {
            reader.releaseLock();
        } catch {}
    }
}

function handle(line) {
    const p = line.split(',');
    if (p[0] === 'STIM') {
        $('counter').textContent = `Trial: ${p[1]}/20`;
        $('status').textContent = 'Respond now';
        lights(p[2]);
    } else if (p[0] === 'RESULT') {
        rows.push({
            trial: +p[1],
            stimulus: p[2],
            response: p[3],
            rt: +p[4],
            correct: p[5] === '1',
            omission: p[6] === '1'
        });
        lights('');
        $('status').textContent = 'Next stimulus…';
    } else if (p[0] === 'STATE') {
        if (p[1] === 'DONE') finish();
        if (p[1] === 'STOPPED') {
            lights('');
            show('intro');
        }
    } else if (p[0] === 'READY') msg('Arduino ready');
}

function finish() {
    show('results');
    const correct = rows.filter(x => x.correct),
        valid = correct.map(x => x.rt);
    $('rt').textContent = valid.length ? (valid.reduce((a, b) => a + b, 0) / valid.length).toFixed(1) + ' ms' : '—';
    $('accuracy').textContent = (rows.length ? 100 * correct.length / rows.length : 0).toFixed(1) + '%';
    $('omissions').textContent = rows.filter(x => x.omission).length;
    $('commissions').textContent = rows.filter(x => !x.omission && !x.correct).length;
}
$('connect').onclick = connect;
$('start').onclick = async () => {
    rows = [];
    show('test');
    lights('');
    $('counter').textContent = 'Trial: 0/20';
    $('status').textContent = 'Waiting';
    try {
        await send('START');
    } catch (e) {
        msg(e.message);
        show('intro');
    }
};
$('stop').onclick = async () => {
    try {
        await send('STOP');
    } catch (e) {
        msg(e.message);
    }
    show('intro');
};
$('restart').onclick = () => show('intro');
$('csv').onclick = () => {
    const csv = ['trial,stimulus,response,rt_ms,correct,omission', ...rows.map(r => `${r.trial},${r.stimulus},${r.response},${r.rt},${+r.correct},${+r.omission}`)].join('\n');
    const a = document.createElement('a');
    const url = URL.createObjectURL(new Blob([csv], {
        type: 'text/csv;charset=utf-8'
    }));
    a.href = url;
    a.download = 'determination-results.csv';
    a.click();
    setTimeout(() => URL.revokeObjectURL(url), 1000);
};