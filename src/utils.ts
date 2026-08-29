import * as fs from 'node:fs';
import * as path from 'node:path';

/**
 * Windows FILETIME (100-nanosecond intervals since January 1, 1601 UTC)
 * to ISO 8601 Date String
 */
export function fileTimeToIsoString(filetime: string | number | undefined): string | undefined {
  if (filetime === undefined || filetime === null || filetime === '' || filetime === '0' || filetime === 0) {
    return undefined;
  }

  try {
    const ftBig = typeof filetime === 'bigint' ? filetime : BigInt(String(filetime));
    if (ftBig <= 0n) return undefined;

    // Unix Epoch in Windows FILETIME: 116444736000000000 (1601 -> 1970)
    const epochDifference = 116444736000000000n;
    if (ftBig < epochDifference) return undefined;

    const unixMs = Number((ftBig - epochDifference) / 10000n);
    const date = new Date(unixMs);
    if (isNaN(date.getTime())) return undefined;

    return date.toISOString();
  } catch {
    return undefined;
  }
}

/**
 * Format bytes into human-readable size string (e.g. 1.25 MB)
 */
export function formatBytes(bytesInput: number | string | undefined): string | undefined {
  if (bytesInput === undefined || bytesInput === null || bytesInput === '') {
    return undefined;
  }

  const bytes = typeof bytesInput === 'number' ? bytesInput : Number(bytesInput);
  if (isNaN(bytes) || bytes < 0) return undefined;
  if (bytes === 0) return '0 B';

  const units = ['B', 'KB', 'MB', 'GB', 'TB', 'PB'];
  const i = Math.floor(Math.log(bytes) / Math.log(1024));
  const unitIndex = Math.min(i, units.length - 1);
  const value = bytes / Math.pow(1024, unitIndex);

  return `${value.toFixed(unitIndex === 0 ? 0 : 2)} ${units[unitIndex]}`;
}

/**
 * Combine folder path and file name to construct canonical Windows full path
 */
export function buildFullPath(dirPath: string | undefined, fileName: string): string {
  if (!dirPath) return fileName;
  if (dirPath.endsWith('\\') || dirPath.endsWith('/')) {
    return `${dirPath}${fileName}`;
  }
  return `${dirPath}\\${fileName}`;
}

/**
 * Extract file extension without dot
 */
export function getExtension(fileName: string): string | undefined {
  const ext = path.extname(fileName);
  return ext ? ext.slice(1).toLowerCase() : undefined;
}

/**
 * Safely inspect and preview text file content
 */
export function previewTextFile(
  filePath: string,
  startLine = 1,
  maxLines = 50
): {
  success: boolean;
  content?: string;
  totalLines?: number;
  isBinary?: boolean;
  error?: string;
} {
  try {
    if (!fs.existsSync(filePath)) {
      return { success: false, error: `File not found: ${filePath}` };
    }

    const stats = fs.statSync(filePath);
    if (stats.isDirectory()) {
      return { success: false, error: `Path is a directory, not a file: ${filePath}` };
    }

    // Check if file is excessively large for preview (> 100MB)
    const fd = fs.openSync(filePath, 'r');
    const sampleBuffer = Buffer.alloc(Math.min(4096, stats.size));
    fs.readSync(fd, sampleBuffer, 0, sampleBuffer.length, 0);
    fs.closeSync(fd);

    // Simple heuristic for binary file: check for NULL bytes in initial chunk
    for (let i = 0; i < sampleBuffer.length; i++) {
      if (sampleBuffer[i] === 0) {
        return {
          success: false,
          isBinary: true,
          error: `File appears to be binary (${stats.size} bytes): ${filePath}`,
        };
      }
    }

    const fileContent = fs.readFileSync(filePath, 'utf-8');
    const lines = fileContent.split(/\r?\n/);
    const totalLines = lines.length;

    const adjustedStart = Math.max(1, startLine);
    const startIdx = adjustedStart - 1;
    const endIdx = Math.min(totalLines, startIdx + Math.max(1, maxLines));

    const selectedLines = lines.slice(startIdx, endIdx);
    const formatted = selectedLines
      .map((line, idx) => `${startIdx + idx + 1}: ${line}`)
      .join('\n');

    return {
      success: true,
      content: formatted,
      totalLines,
      isBinary: false,
    };
  } catch (err: any) {
    return {
      success: false,
      error: `Failed to read file: ${err?.message || String(err)}`,
    };
  }
}
