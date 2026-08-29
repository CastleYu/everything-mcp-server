#!/usr/bin/env node

import * as net from 'node:net';
import * as readline from 'node:readline';

const PIPE_NAME = process.env.EVERYTHING_MCP_PIPE || '\\\\.\\pipe\\EverythingMCP';

console.error(`[everything-mcp-bridge] Connecting to native Everything MCP Plugin at: ${PIPE_NAME}`);

const socket = net.connect(PIPE_NAME, () => {
  console.error('[everything-mcp-bridge] Connected to Everything Native MCP Plugin successfully.');
});

socket.on('error', (err) => {
  console.error(`[everything-mcp-bridge] Connection error: ${err.message}`);
  console.error('[everything-mcp-bridge] Please make sure Everything 1.5 is running and mcp_server64.dll is placed in the Plugins folder.');
  process.exit(1);
});

// Forward stdin -> named pipe
const rl = readline.createInterface({
  input: process.stdin,
  terminal: false,
});

rl.on('line', (line) => {
  if (line.trim()) {
    socket.write(line + '\n');
  }
});

// Forward named pipe -> stdout
const socketRl = readline.createInterface({
  input: socket,
  terminal: false,
});

socketRl.on('line', (line) => {
  if (line.trim()) {
    process.stdout.write(line + '\n');
  }
});

socket.on('close', () => {
  console.error('[everything-mcp-bridge] Connection closed by Everything.');
  process.exit(0);
});
