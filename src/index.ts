#!/usr/bin/env node

import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { StdioServerTransport } from '@modelcontextprotocol/sdk/server/stdio.js';
import * as dotenv from 'dotenv';
import { EverythingClient } from './client.js';
import { registerEverythingTools } from './tools.js';

// Load optional .env file
dotenv.config();

async function main() {
  const client = new EverythingClient();

  const server = new McpServer({
    name: 'everything-mcp-server',
    version: '1.0.0',
  });

  registerEverythingTools(server, client);

  const transport = new StdioServerTransport();
  await server.connect(transport);

  console.error(
    `[everything-mcp] Everything MCP Server initialized. Target URL: ${client.getBaseUrl()}`
  );
}

main().catch((err) => {
  console.error('[everything-mcp] Fatal error during startup:', err);
  process.exit(1);
});
