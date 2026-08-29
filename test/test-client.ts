import { EverythingClient } from '../src/client.js';
import { previewTextFile } from '../src/utils.js';

async function runTests() {
  console.log('=== Everything MCP Client Test Suite ===\n');

  const client = new EverythingClient();
  console.log(`[1] Detected Base URL: ${client.getBaseUrl()}`);

  // Test 1: Status
  console.log('\n[2] Testing Everything status...');
  const status = await client.getStatus();
  console.log('Status result:', JSON.stringify(status, null, 2));

  if (!status.connected) {
    console.error('FAILED: Could not connect to Everything instance.');
    process.exit(1);
  }

  // Test 2: Search
  console.log('\n[3] Testing everything_search for "*.md"...');
  const searchRes = await client.search({
    query: '*.md',
    maxResults: 5,
    sort: 'date_modified',
    ascending: false,
  });
  console.log(`Found ${searchRes.totalResults} total results, returned ${searchRes.returnedCount} items in ${searchRes.executionTimeMs}ms:`);
  for (const item of searchRes.items) {
    console.log(`  - [${item.type}] ${item.fullPath} (${item.sizeFormatted || 'N/A'}, ${item.dateModified || 'N/A'})`);
  }

  // Test 3: Specific File Info
  if (searchRes.items.length > 0) {
    const testPath = searchRes.items[0].fullPath;
    console.log(`\n[4] Testing getFileInfo for: ${testPath}`);
    const fileInfo = await client.getFileInfo(testPath);
    console.log('File info result:', JSON.stringify(fileInfo, null, 2));

    console.log(`\n[5] Testing previewTextFile for: ${testPath}`);
    const preview = previewTextFile(testPath, 1, 10);
    console.log('Preview success:', preview.success);
    if (preview.success) {
      console.log('Preview snippet:\n' + preview.content);
    }
  }

  console.log('\n=== All Tests Passed Successfully! ===');
}

runTests().catch((err) => {
  console.error('Test suite failed:', err);
  process.exit(1);
});
