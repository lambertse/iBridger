import * as net from 'net';
import * as os from 'os';
import * as path from 'path';
import * as fs from 'fs';
import { UnixSocketConnection } from '../../src/transport/unix-socket';

function tempSocketPath(): string {
  return path.join(os.tmpdir(), `ibridger_test_${process.pid}_${Date.now()}.sock`);
}

function createEchoServer(sockPath: string): Promise<net.Server> {
  return new Promise((resolve) => {
    const server = net.createServer((socket) => {
      socket.pipe(socket);  // echo everything back
    });
    server.listen({ path: sockPath }, () => resolve(server));
  });
}

function stopServer(server: net.Server, sockPath: string): Promise<void> {
  return new Promise((resolve) => {
    server.close(() => {
      try { fs.unlinkSync(sockPath); } catch {}
      resolve();
    });
  });
}

// ─── Connect and basic send/recv ──────────────────────────────────────────────

it('connects and roundtrips data', async () => {
  const sockPath = tempSocketPath();
  const server = await createEchoServer(sockPath);
  try {
    const conn = await UnixSocketConnection.connect(sockPath);
    const data = Buffer.from('hello transport');
    await conn.send(data);
    const received = await conn.recv(data.length);
    expect(received).toEqual(data);
    conn.close();
  } finally {
    await stopServer(server, sockPath);
  }
});

// ─── recv waits for bytes across multiple data events ─────────────────────────

it('recv assembles bytes across fragmented data events', async () => {
  const sockPath = tempSocketPath();
  // Server sends two chunks with a delay between them.
  const server = net.createServer((socket) => {
    socket.write(Buffer.from('hel'));
    setTimeout(() => socket.write(Buffer.from('lo!')), 30);
  });
  await new Promise<void>((r) => server.listen({ path: sockPath }, () => r()));

  try {
    const conn = await UnixSocketConnection.connect(sockPath);
    const result = await conn.recv(6);
    expect(result.toString()).toBe('hello!');
    conn.close();
  } finally {
    await stopServer(server, sockPath);
  }
});

// ─── Multiple sequential recvs ────────────────────────────────────────────────

it('handles multiple sequential recv calls', async () => {
  const sockPath = tempSocketPath();
  const server = await createEchoServer(sockPath);
  try {
    const conn = await UnixSocketConnection.connect(sockPath);
    await conn.send(Buffer.from('abcdef'));
    const first  = await conn.recv(3);
    const second = await conn.recv(3);
    expect(first.toString()).toBe('abc');
    expect(second.toString()).toBe('def');
    conn.close();
  } finally {
    await stopServer(server, sockPath);
  }
});

// ─── isConnected reflects state ───────────────────────────────────────────────

it('isConnected is true after connect and false after close', async () => {
  const sockPath = tempSocketPath();
  const server = await createEchoServer(sockPath);
  try {
    const conn = await UnixSocketConnection.connect(sockPath);
    expect(conn.isConnected).toBe(true);
    conn.close();
    expect(conn.isConnected).toBe(false);
  } finally {
    await stopServer(server, sockPath);
  }
});

// ─── onDisconnect fires when server closes ────────────────────────────────────

it('calls onDisconnect when the server closes the connection', async () => {
  const sockPath = tempSocketPath();

  // Server that tracks its sockets so we can destroy them forcefully.
  const serverSockets = new Set<net.Socket>();
  const server = net.createServer((s) => {
    serverSockets.add(s);
    s.on('close', () => serverSockets.delete(s));
    s.pipe(s);
  });
  await new Promise<void>((r) => server.listen({ path: sockPath }, () => r()));

  const conn = await UnixSocketConnection.connect(sockPath);

  let fired = false;
  conn.onDisconnect = () => { fired = true; };

  // Destroy all server-side sockets to force the client to see a close event.
  for (const s of serverSockets) s.destroy();
  await new Promise<void>((r) => server.close(() => r()));
  try { fs.unlinkSync(sockPath); } catch {}

  await new Promise<void>((r) => setTimeout(r, 50));
  expect(fired).toBe(true);
  expect(conn.isConnected).toBe(false);
});

// ─── Connection refused ───────────────────────────────────────────────────────

it('rejects when server is not listening', async () => {
  await expect(
    UnixSocketConnection.connect('/tmp/ibridger_no_server_ever.sock')
  ).rejects.toThrow();
});
