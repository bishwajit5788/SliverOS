/**
 * notifications.js
 * Status badges and notification helpers.
 */

export class NotificationUI {
  constructor(badgeElement, safetyElement) {
    this.badge = badgeElement;
    this.safety = safetyElement;
  }

  setStatus(state, message = '') {
    if (!this.badge) return;

    this.badge.className = 'badge badge-status';
    this.badge.textContent = state;

    switch (state) {
      case 'READY':
        this.badge.classList.add('status-ready');
        break;
      case 'FLASHING':
        this.badge.classList.add('status-flashing');
        break;
      case 'ERROR':
      case 'DISCONNECTED':
        this.badge.classList.add('status-error');
        break;
      default:
        break;
    }

    if (this.safety && message) {
      this.safety.textContent = message;
    }
  }
}
