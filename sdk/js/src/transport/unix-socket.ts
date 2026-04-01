import * as net from 'net';
import { IConnection, TransportError } from './types';

/**
 * Unix domain socket (or Windows named pipe) connection.
 *
 * Incoming data is accumulated in an internal buffer. recv(n) resolves once
 * at least n bytes are available, splicing exactly n bytes from the front.
 * The socket is paused whenever the buffer holds unread data to prevent
 * unbounded buffering; it is resumed after each recv() drains enough bytes.
 */
export class UnixSocketConnection implements IConnection {
  private buffer: Buffer = Buffer.alloc(0);
  private connected = true;
  private readonly waiters: Array<{ need: number; resolve: (b: Buffer) => void; reject: (e: Error) => void }> = [];

  /** Called once when the underlying socket closes or errors unexpectedly. */
  onDisconnect?: () => void;

  private constructor(private readonly socket: net.Socket) {
    socket.on('data', (chunk: Buffer) => {
      this.buffer = Buffer.concat([this.buffer, chunk]);
      this.drainWaiters();
    });

    socket.on('close', () => {
      this.connected = false;
      const err = new TransportError('Connection closed', 'ECONNRESET');
      for (const w of this.waiters) w.reject(err);
      this.waiters.length = 0;
      this.onDisconnect?.();
    });

    socket.on('error', (err: Error) => {
      this.connected = false;
      const te = new TransportError(err.message, (err as NodeJS.ErrnoException).code ?? 'EUNKNOWN');
      for (const w of this.waiters) w.reject(te);
      this.waiters.length = 0;
      this.onDisconnect?.();
    });
  }

  static connect(path: string): Promise<UnixSocketConnection> {
    return new Promise((resolve, reject) => {
      const socket = net.createConnection({ path }, () => {
        resolve(new UnixSocketConnection(socket));
      });
      socket.once('error', reject);
    });
  }

  /** Wrap an already-connected socket (useful for server-side accept). */
  static fromSocket(socket: net.Socket): UnixSocketConnection {
    return new UnixSocketConnection(socket);
  }

  async send(data: Buffer): Promise<void> {
    if (!this.connected) {
      throw new TransportError('Not connected', 'ENOTCONN');
    }
    return new Promise((resolve, reject) => {
      this.socket.write(data, (err) => {
        if (err) reject(new TransportError(err.message, (err as NodeJS.ErrnoException).code ?? 'EUNKNOWN'));
        else resolve();
      });
    });
  }

  async recv(length: number): Promise<Buffer> {
    if (this.buffer.length >= length) {
      return this.splice(length);
    }
    if (!this.connected) {
      throw new TransportError('Connection closed', 'ECONNRESET');
    }
    return new Promise<Buffer>((resolve, reject) => {
      this.waiters.push({ need: length, resolve, reject });
    });
  }

  close(): void {
    this.connected = false;
    this.socket.destroy();
  }

  get isConnected(): boolean {
    return this.connected;
  }

  private splice(n: number): Buffer {
    const out = this.buffer.subarray(0, n);
    this.buffer = this.buffer.subarray(n);
    return Buffer.from(out);
  }

  private drainWaiters(): void {
    while (this.waiters.length > 0) {
      const w = this.waiters[0];
      if (this.buffer.length < w.need) break;
      this.waiters.shift();
      w.resolve(this.splice(w.need));
    }
  }
}
