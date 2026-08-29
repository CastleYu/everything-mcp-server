import * as fs from 'node:fs';
import * as path from 'node:path';
import {
  EverythingQueryOptions,
  EverythingRawSearchResponse,
  EverythingSearchResponse,
  EverythingSearchResultItem,
  EverythingStatusResult,
} from './types.js';
import { buildFullPath, fileTimeToIsoString, formatBytes, getExtension } from './utils.js';

export interface EverythingClientConfig {
  baseUrl?: string;
  username?: string;
  password?: string;
  timeoutMs?: number;
}

export class EverythingClient {
  private baseUrl: string;
  private username?: string;
  private password?: string;
  private timeoutMs: number;

  constructor(config?: EverythingClientConfig) {
    this.username = config?.username || process.env.EVERYTHING_HTTP_USERNAME;
    this.password = config?.password || process.env.EVERYTHING_HTTP_PASSWORD;
    this.timeoutMs = config?.timeoutMs || 8000;

    // Detect or configure baseUrl
    if (config?.baseUrl) {
      this.baseUrl = config.baseUrl;
    } else if (process.env.EVERYTHING_HTTP_URL) {
      this.baseUrl = process.env.EVERYTHING_HTTP_URL;
    } else {
      this.baseUrl = this.autoDetectBaseUrl();
    }
  }

  /**
   * Attempt to locate Plugins.ini or default port (8088 / 80)
   */
  private autoDetectBaseUrl(): string {
    const candidatePaths = [
      'D:\\Set\\Everything\\Everything\\Plugins.ini',
      path.join(process.env.APPDATA || '', 'Everything', 'Plugins.ini'),
      'C:\\Program Files\\Everything\\Plugins.ini',
      'C:\\Program Files (x86)\\Everything\\Plugins.ini',
    ];

    for (const iniPath of candidatePaths) {
      try {
        if (fs.existsSync(iniPath)) {
          const content = fs.readFileSync(iniPath, 'utf-8');
          const portMatch = content.match(/port\s*=\s*(\d+)/i);
          if (portMatch && portMatch[1]) {
            return `http://127.0.0.1:${portMatch[1]}`;
          }
        }
      } catch {
        // ignore and fallback
      }
    }

    return 'http://127.0.0.1:8088';
  }

  public getBaseUrl(): string {
    return this.baseUrl;
  }

  public setBaseUrl(url: string): void {
    this.baseUrl = url;
  }

  private getAuthHeader(): Record<string, string> {
    if (this.username && this.password) {
      const token = Buffer.from(`${this.username}:${this.password}`).toString('base64');
      return { Authorization: `Basic ${token}` };
    }
    return {};
  }

  /**
   * Execute general search query against Everything HTTP Server
   */
  public async search(options: EverythingQueryOptions): Promise<EverythingSearchResponse> {
    const startTime = Date.now();

    let queryStr = options.query?.trim() || '';

    // Apply type filter if specified
    if (options.typeFilter === 'files' && !queryStr.includes('file:')) {
      queryStr = queryStr ? `file: ${queryStr}` : 'file:';
    } else if (options.typeFilter === 'folders' && !queryStr.includes('folder:')) {
      queryStr = queryStr ? `folder: ${queryStr}` : 'folder:';
    }

    const params = new URLSearchParams();
    params.set('search', queryStr);
    params.set('json', '1');
    params.set('count', String(Math.min(options.maxResults ?? 30, 500)));
    params.set('offset', String(Math.max(options.offset ?? 0, 0)));

    // Request full column metadata
    params.set('path_column', '1');
    params.set('size_column', '1');
    params.set('date_modified_column', '1');
    params.set('date_created_column', '1');
    params.set('attributes_column', '1');

    if (options.sort) {
      params.set('sort', options.sort);
      params.set('ascending', options.ascending === false ? '0' : '1');
    }

    if (options.matchCase) params.set('case', '1');
    if (options.matchWholeWord) params.set('wholeword', '1');
    if (options.matchPath) params.set('path', '1');
    if (options.regex) params.set('regex', '1');

    const targetUrl = `${this.baseUrl}/?${params.toString()}`;

    const controller = new AbortController();
    const timeoutId = setTimeout(() => controller.abort(), this.timeoutMs);

    try {
      const response = await fetch(targetUrl, {
        method: 'GET',
        headers: {
          Accept: 'application/json',
          ...this.getAuthHeader(),
        },
        signal: controller.signal,
      });

      if (!response.ok) {
        throw new Error(
          `Everything HTTP Server returned status ${response.status}: ${response.statusText}`
        );
      }

      const rawData = (await response.json()) as EverythingRawSearchResponse;
      const executionTimeMs = Date.now() - startTime;

      const items: EverythingSearchResultItem[] = (rawData.results || []).map((item) => {
        const fullPath = buildFullPath(item.path, item.name);
        const sizeBytes =
          item.size !== undefined && item.size !== '' ? Number(item.size) : undefined;
        const attrNum =
          item.attributes !== undefined && item.attributes !== ''
            ? Number(item.attributes)
            : undefined;

        return {
          type: item.type,
          name: item.name,
          path: item.path || '',
          fullPath,
          sizeBytes,
          sizeFormatted: formatBytes(sizeBytes),
          dateModified: fileTimeToIsoString(item.date_modified),
          dateCreated: fileTimeToIsoString(item.date_created),
          extension: item.type === 'file' ? getExtension(item.name) : undefined,
          attributes: attrNum,
        };
      });

      return {
        totalResults: rawData.totalResults || 0,
        returnedCount: items.length,
        offset: options.offset ?? 0,
        query: queryStr,
        items,
        executionTimeMs,
      };
    } catch (err: any) {
      if (err.name === 'AbortError') {
        throw new Error(`Everything HTTP query timed out after ${this.timeoutMs}ms (${targetUrl})`);
      }
      throw new Error(
        `Failed to connect to Everything HTTP Server (${this.baseUrl}): ${err?.message || String(err)}. Please ensure Everything is running and the HTTP Server plugin is enabled.`
      );
    } finally {
      clearTimeout(timeoutId);
    }
  }

  /**
   * Fetch specific file or folder indexing metadata
   */
  public async getFileInfo(targetPath: string): Promise<{
    found: boolean;
    everythingMetadata?: EverythingSearchResultItem;
    filesystemStats?: {
      exists: boolean;
      isDirectory?: boolean;
      sizeBytes?: number;
      sizeFormatted?: string;
      dateModified?: string;
      dateCreated?: string;
    };
  }> {
    const cleanPath = targetPath.trim().replace(/^"|"$/g, '');

    // Search exact path in Everything
    const searchRes = await this.search({
      query: `exact:"${cleanPath}"`,
      maxResults: 1,
      matchPath: true,
    });

    let everythingMetadata: EverythingSearchResultItem | undefined = searchRes.items[0];

    // Check filesystem status as well
    let filesystemStats: any = { exists: false };
    try {
      if (fs.existsSync(cleanPath)) {
        const stats = fs.statSync(cleanPath);
        filesystemStats = {
          exists: true,
          isDirectory: stats.isDirectory(),
          sizeBytes: stats.size,
          sizeFormatted: formatBytes(stats.size),
          dateModified: stats.mtime.toISOString(),
          dateCreated: stats.birthtime.toISOString(),
        };
      }
    } catch {
      filesystemStats = { exists: false };
    }

    return {
      found: !!everythingMetadata || filesystemStats.exists,
      everythingMetadata,
      filesystemStats,
    };
  }

  /**
   * Check connection status to Everything
   */
  public async getStatus(): Promise<EverythingStatusResult> {
    const startTime = Date.now();
    try {
      const res = await this.search({
        query: '',
        maxResults: 1,
      });

      const latencyMs = Date.now() - startTime;
      return {
        connected: true,
        baseUrl: this.baseUrl,
        latencyMs,
        totalIndexedItems: res.totalResults,
      };
    } catch (err: any) {
      return {
        connected: false,
        baseUrl: this.baseUrl,
        error: err?.message || String(err),
      };
    }
  }
}
