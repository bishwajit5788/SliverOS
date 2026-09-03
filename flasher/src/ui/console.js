/**
 * console.js
 * Expandable developer console with level filtering and autoscroll.
 */

export class ConsoleUI {
  constructor(containerElement) {
    this.container = containerElement;
    this.logs = [];
    this.currentFilter = 'ALL';
  }

  log(message, level = 'INFO') {
    // If message starts with tag like [DEBUG], extract level
    let inferredLevel = level;
    let text = message;

    const match = message.match(/^\[(INFO|WARN|ERROR|DEBUG)\]\s*(.*)$/);
    if (match) {
      inferredLevel = match[1];
      text = match[2];
    }

    const entry = {
      id: Date.now() + Math.random(),
      time: new Date().toLocaleTimeString(),
      level: inferredLevel,
      text
    };

    this.logs.push(entry);
    this.renderEntry(entry);
  }

  renderEntry(entry) {
    if (this.currentFilter !== 'ALL' && entry.level !== this.currentFilter) {
      return;
    }

    const row = document.createElement('div');
    row.className = 'log-entry';
    row.dataset.level = entry.level;

    row.innerHTML = `
      <span class="log-time">[${entry.time}]</span>
      <span class="log-level log-level-${entry.level}">[${entry.level}]</span>
      <span class="log-msg">${this.escapeHTML(entry.text)}</span>
    `;

    this.container.appendChild(row);
    this.container.scrollTop = this.container.scrollHeight;
  }

  setFilter(filterLevel) {
    this.currentFilter = filterLevel;
    this.container.innerHTML = '';
    for (const entry of this.logs) {
      this.renderEntry(entry);
    }
  }

  clear() {
    this.logs = [];
    this.container.innerHTML = '';
  }

  escapeHTML(str) {
    return str.replace(/[&<>'"]/g, tag => ({
      '&': '&amp;',
      '<': '&lt;',
      '>': '&gt;',
      "'": '&#39;',
      '"': '&quot;'
    }[tag] || tag));
  }
}
