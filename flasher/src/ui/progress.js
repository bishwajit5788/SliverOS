/**
 * progress.js
 * Multi-bar progress renderer for overall and per-image flashing metrics.
 */

export class ProgressUI {
  constructor(overallBar, overallPct, overallLabel, metricsContainer, imageListContainer) {
    this.overallBar = overallBar;
    this.overallPct = overallPct;
    this.overallLabel = overallLabel;
    this.metrics = metricsContainer;
    this.imageList = imageListContainer;
    this.imageBars = [];
  }

  setupImages(images) {
    this.imageList.innerHTML = '';
    this.imageBars = [];

    images.forEach((img, idx) => {
      const item = document.createElement('div');
      item.className = 'image-progress-item';
      item.innerHTML = `
        <div class="image-progress-header">
          <span>${img.name} (0x${img.offset.toString(16).toUpperCase()})</span>
          <span id="imgPct_${idx}">0%</span>
        </div>
        <div class="image-progress-bar">
          <div class="image-progress-bar-fill" id="imgBar_${idx}" style="width: 0%;"></div>
        </div>
      `;
      this.imageList.appendChild(item);

      this.imageBars.push({
        bar: item.querySelector(`#imgBar_${idx}`),
        pct: item.querySelector(`#imgPct_${idx}`)
      });
    });
  }

  update(telemetry) {
    // Overall progress
    if (this.overallBar) {
      this.overallBar.style.width = `${telemetry.overallPct}%`;
    }
    if (this.overallPct) {
      this.overallPct.textContent = `${telemetry.overallPct}%`;
    }
    if (this.overallLabel) {
      this.overallLabel.textContent = `Writing ${telemetry.imageName}...`;
    }

    // Current image progress
    if (this.imageBars[telemetry.imageIndex]) {
      const imgUi = this.imageBars[telemetry.imageIndex];
      imgUi.bar.style.width = `${telemetry.imagePct}%`;
      imgUi.pct.textContent = `${telemetry.imagePct}%`;
    }

    // Previous images marked 100%
    for (let i = 0; i < telemetry.imageIndex; i++) {
      if (this.imageBars[i]) {
        this.imageBars[i].bar.style.width = '100%';
        this.imageBars[i].pct.textContent = '100%';
      }
    }

    // Metrics text
    const metricSpeed = document.getElementById('metricSpeed');
    const metricTransferred = document.getElementById('metricTransferred');
    const metricEta = document.getElementById('metricEta');

    if (metricSpeed) metricSpeed.textContent = `${telemetry.speedKBps} KB/s`;
    if (metricTransferred) {
      const kbDone = (telemetry.bytesTransferred / 1024).toFixed(0);
      const kbTot = (telemetry.totalBytes / 1024).toFixed(0);
      metricTransferred.textContent = `${kbDone} / ${kbTot} KB`;
    }
    if (metricEta) metricEta.textContent = `ETA: ${telemetry.remainingSec}s`;
  }

  reset() {
    if (this.overallBar) this.overallBar.style.width = '0%';
    if (this.overallPct) this.overallPct.textContent = '0%';
    if (this.overallLabel) this.overallLabel.textContent = 'Overall Progress';
    this.imageList.innerHTML = '';
    this.imageBars = [];
  }
}
