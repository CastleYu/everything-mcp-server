/**
 * Everything Raw HTTP JSON Response Models
 */
export interface EverythingRawResultItem {
  type: 'file' | 'folder';
  name: string;
  path?: string;
  size?: string | number;
  date_modified?: string | number;
  date_created?: string | number;
  attributes?: string | number;
}

export interface EverythingRawSearchResponse {
  totalResults: number;
  results?: EverythingRawResultItem[];
}

/**
 * Formatted search result item presented to LLM/MCP client
 */
export interface EverythingSearchResultItem {
  type: 'file' | 'folder';
  name: string;
  path: string;
  fullPath: string;
  sizeBytes?: number;
  sizeFormatted?: string;
  dateModified?: string;
  dateCreated?: string;
  extension?: string;
  attributes?: number;
}

export interface EverythingSearchResponse {
  totalResults: number;
  returnedCount: number;
  offset: number;
  query: string;
  items: EverythingSearchResultItem[];
  executionTimeMs?: number;
}

/**
 * Client search query options
 */
export type SortOption =
  | 'name'
  | 'path'
  | 'size'
  | 'extension'
  | 'date_modified'
  | 'date_created'
  | 'attributes';

export type TypeFilter = 'all' | 'files' | 'folders';

export interface EverythingQueryOptions {
  query: string;
  maxResults?: number;
  offset?: number;
  sort?: SortOption;
  ascending?: boolean;
  matchCase?: boolean;
  matchWholeWord?: boolean;
  matchPath?: boolean;
  regex?: boolean;
  typeFilter?: TypeFilter;
}

/**
 * Helper search input for files
 */
export interface FindFilesOptions {
  name?: string;
  extension?: string;
  directory?: string;
  minSize?: string;
  maxSize?: string;
  modifiedAfter?: string;
  modifiedBefore?: string;
  maxResults?: number;
  offset?: number;
  sort?: SortOption;
  ascending?: boolean;
}

/**
 * Helper search input for folders
 */
export interface FindFoldersOptions {
  name?: string;
  parentDirectory?: string;
  maxResults?: number;
  offset?: number;
  sort?: SortOption;
  ascending?: boolean;
}

export interface EverythingStatusResult {
  connected: boolean;
  baseUrl: string;
  latencyMs?: number;
  totalIndexedItems?: number;
  version?: string;
  error?: string;
  processInfo?: {
    isRunning: boolean;
    processPath?: string;
  };
}
