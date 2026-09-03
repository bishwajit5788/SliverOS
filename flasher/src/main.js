/**
 * main.js
 * Application entrypoint for the MicroKernel OS Web Serial Flasher.
 */

import { Dashboard } from './ui/dashboard.js';

window.addEventListener('DOMContentLoaded', async () => {
  const dashboard = new Dashboard();
  await dashboard.init();
});
