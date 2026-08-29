import { McpServer } from '@modelcontextprotocol/sdk/server/mcp.js';
import { z } from 'zod';
import { EverythingClient } from './client.js';
import { previewTextFile } from './utils.js';

export function registerEverythingTools(server: McpServer, client: EverythingClient): void {
  /**
   * Tool 1: everything_search
   * Powerful, ultra-fast file and directory search with full Everything query syntax support
   */
  server.tool(
    'everything_search',
    'Search files and folders using voidtools Everything ultra-fast search engine. Supports full Everything syntax (e.g. wildcards, ext:ts;js, size:>10mb, dm:today, regex, etc.).',
    {
      query: z
        .string()
        .describe(
          'Everything search query expression (e.g. "*.pdf", "ext:docx;xlsx", "size:>100mb", "dm:today project")'
        ),
      max_results: z
        .number()
        .min(1)
        .max(500)
        .optional()
        .default(30)
        .describe('Maximum number of results to return (default: 30, max: 500)'),
      offset: z
        .number()
        .min(0)
        .optional()
        .default(0)
        .describe('Offset / pagination start index (default: 0)'),
      sort: z
        .enum([
          'name',
          'path',
          'size',
          'extension',
          'date_modified',
          'date_created',
          'attributes',
        ])
        .optional()
        .describe('Field to sort results by'),
      ascending: z
        .boolean()
        .optional()
        .default(true)
        .describe('Sort ascending (true) or descending (false)'),
      match_case: z.boolean().optional().default(false).describe('Match case sensitivity'),
      match_whole_word: z.boolean().optional().default(false).describe('Match whole words only'),
      match_path: z.boolean().optional().default(false).describe('Match full path instead of just filename'),
      regex: z.boolean().optional().default(false).describe('Enable regular expression mode'),
      type_filter: z
        .enum(['all', 'files', 'folders'])
        .optional()
        .default('all')
        .describe('Filter by item type: "all", "files", or "folders"'),
    },
    async (args) => {
      try {
        const results = await client.search({
          query: args.query,
          maxResults: args.max_results,
          offset: args.offset,
          sort: args.sort,
          ascending: args.ascending,
          matchCase: args.match_case,
          matchWholeWord: args.match_whole_word,
          matchPath: args.match_path,
          regex: args.regex,
          typeFilter: args.type_filter,
        });

        return {
          content: [
            {
              type: 'text',
              text: JSON.stringify(results, null, 2),
            },
          ],
        };
      } catch (err: any) {
        return {
          isError: true,
          content: [
            {
              type: 'text',
              text: `Search error: ${err?.message || String(err)}`,
            },
          ],
        };
      }
    }
  );

  /**
   * Tool 2: everything_find_files
   * Structured file finder helper
   */
  server.tool(
    'everything_find_files',
    'Find files by filename, extension, directory location, date range, and size range without manually constructing complex Everything query syntax.',
    {
      name: z.string().optional().describe('Filename keyword or pattern (e.g. "report", "*.test.ts")'),
      extension: z
        .string()
        .optional()
        .describe('File extension or semicolon-separated extensions (e.g. "pdf" or "ts;js;json")'),
      directory: z
        .string()
        .optional()
        .describe('Parent directory path to restrict search (e.g. "D:\\Projects" or "C:\\Users")'),
      min_size: z.string().optional().describe('Minimum size (e.g. "1MB", "500KB", "2GB")'),
      max_size: z.string().optional().describe('Maximum size (e.g. "10MB", "1GB")'),
      modified_after: z
        .string()
        .optional()
        .describe('Modified after date (e.g. "today", "yesterday", "thisweek", "2026-01-01")'),
      modified_before: z.string().optional().describe('Modified before date (e.g. "2026-06-01")'),
      max_results: z.number().min(1).max(500).optional().default(30),
      offset: z.number().min(0).optional().default(0),
      sort: z
        .enum([
          'name',
          'path',
          'size',
          'extension',
          'date_modified',
          'date_created',
          'attributes',
        ])
        .optional(),
      ascending: z.boolean().optional().default(true),
    },
    async (args) => {
      try {
        const queryParts: string[] = ['file:'];

        if (args.directory) {
          const cleanDir = args.directory.trim().replace(/^"|"$/g, '');
          queryParts.push(`"${cleanDir}"`);
        }

        if (args.extension) {
          const exts = args.extension.replace(/\./g, '').trim();
          queryParts.push(`ext:${exts}`);
        }

        if (args.name) {
          queryParts.push(args.name.trim());
        }

        if (args.min_size) {
          queryParts.push(`size:>=${args.min_size.trim()}`);
        }

        if (args.max_size) {
          queryParts.push(`size:<=${args.max_size.trim()}`);
        }

        if (args.modified_after) {
          queryParts.push(`dm:>=${args.modified_after.trim()}`);
        }

        if (args.modified_before) {
          queryParts.push(`dm:<=${args.modified_before.trim()}`);
        }

        const constructedQuery = queryParts.join(' ');
        const results = await client.search({
          query: constructedQuery,
          maxResults: args.max_results,
          offset: args.offset,
          sort: args.sort,
          ascending: args.ascending,
          matchPath: !!args.directory,
        });

        return {
          content: [
            {
              type: 'text',
              text: JSON.stringify(
                {
                  constructedQuery,
                  ...results,
                },
                null,
                2
              ),
            },
          ],
        };
      } catch (err: any) {
        return {
          isError: true,
          content: [
            {
              type: 'text',
              text: `Find files error: ${err?.message || String(err)}`,
            },
          ],
        };
      }
    }
  );

  /**
   * Tool 3: everything_find_folders
   * Structured directory/folder finder helper
   */
  server.tool(
    'everything_find_folders',
    'Find folders and directories across the system by name keyword and optional parent directory.',
    {
      name: z.string().optional().describe('Folder name pattern or keyword (e.g. "node_modules", "build", "config*")'),
      parent_directory: z
        .string()
        .optional()
        .describe('Parent directory path to search within (e.g. "D:\\Projects")'),
      max_results: z.number().min(1).max(500).optional().default(30),
      offset: z.number().min(0).optional().default(0),
      sort: z.enum(['name', 'path', 'date_modified', 'date_created']).optional(),
      ascending: z.boolean().optional().default(true),
    },
    async (args) => {
      try {
        const queryParts: string[] = ['folder:'];

        if (args.parent_directory) {
          const cleanDir = args.parent_directory.trim().replace(/^"|"$/g, '');
          queryParts.push(`"${cleanDir}"`);
        }

        if (args.name) {
          queryParts.push(args.name.trim());
        }

        const constructedQuery = queryParts.join(' ');
        const results = await client.search({
          query: constructedQuery,
          maxResults: args.max_results,
          offset: args.offset,
          sort: args.sort,
          ascending: args.ascending,
          matchPath: !!args.parent_directory,
        });

        return {
          content: [
            {
              type: 'text',
              text: JSON.stringify(
                {
                  constructedQuery,
                  ...results,
                },
                null,
                2
              ),
            },
          ],
        };
      } catch (err: any) {
        return {
          isError: true,
          content: [
            {
              type: 'text',
              text: `Find folders error: ${err?.message || String(err)}`,
            },
          ],
        };
      }
    }
  );

  /**
   * Tool 4: everything_get_file_info
   * Detailed metadata inspection for a given file or folder path
   */
  server.tool(
    'everything_get_file_info',
    'Retrieve complete indexing metadata and filesystem stats for a specific file or folder path.',
    {
      path: z.string().describe('Full path to the file or directory (e.g. "C:\\Windows\\notepad.exe")'),
    },
    async (args) => {
      try {
        const info = await client.getFileInfo(args.path);
        return {
          content: [
            {
              type: 'text',
              text: JSON.stringify(info, null, 2),
            },
          ],
        };
      } catch (err: any) {
        return {
          isError: true,
          content: [
            {
              type: 'text',
              text: `Get file info error: ${err?.message || String(err)}`,
            },
          ],
        };
      }
    }
  );

  /**
   * Tool 5: everything_preview_file
   * Safely read text snippet from a file found by search
   */
  server.tool(
    'everything_preview_file',
    'Preview text content of a file located via Everything search (includes line numbering and binary detection).',
    {
      path: z.string().describe('Full path to the text file'),
      start_line: z.number().min(1).optional().default(1).describe('Starting line number (1-indexed, default: 1)'),
      max_lines: z.number().min(1).max(200).optional().default(50).describe('Maximum number of lines to preview (default: 50, max: 200)'),
    },
    async (args) => {
      const preview = previewTextFile(args.path, args.start_line, args.max_lines);
      if (!preview.success) {
        return {
          isError: true,
          content: [
            {
              type: 'text',
              text: preview.error || 'Failed to preview file',
            },
          ],
        };
      }

      return {
        content: [
          {
            type: 'text',
            text: `File: ${args.path}\nLines: ${args.start_line} - ${Math.min(
              (preview.totalLines || 0),
              args.start_line + args.max_lines - 1
            )} (Total: ${preview.totalLines})\n\n${preview.content}`,
          },
        ],
      };
    }
  );

  /**
   * Tool 6: everything_status
   * Health and connectivity status of the Everything instance
   */
  server.tool(
    'everything_status',
    'Check Everything search engine connection status, total indexed items in database, and HTTP server endpoint info.',
    {},
    async () => {
      const status = await client.getStatus();
      return {
        content: [
          {
            type: 'text',
            text: JSON.stringify(status, null, 2),
          },
        ],
      };
    }
  );
}
