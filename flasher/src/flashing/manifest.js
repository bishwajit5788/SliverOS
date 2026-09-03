/**
 * manifest.js
 * Loads, parses, and validates the structured firmware release manifest.
 */

export class ManifestManager {
  constructor(manifestUrl = './firmware/releases.json') {
    this.manifestUrl = manifestUrl;
    this.manifest = null;
  }

  async load() {
    try {
      const resp = await fetch(this.manifestUrl);
      if (!resp.ok) {
        throw new Error(`Failed to fetch firmware manifest: HTTP ${resp.status}`);
      }
      this.manifest = await resp.json();
      this.validate(this.manifest);
      return this.manifest;
    } catch (err) {
      // Fallback relative path check if hosted under subdir
      try {
        const resp2 = await fetch('../firmware/manifests/releases.json');
        if (resp2.ok) {
          this.manifest = await resp2.json();
          this.validate(this.manifest);
          return this.manifest;
        }
      } catch (_) {}
      throw new Error(`Manifest load failure: ${err.message}`);
    }
  }

  validate(manifest) {
    if (!manifest.product || !manifest.releases || !Array.isArray(manifest.releases)) {
      throw new Error('Invalid manifest: missing product or releases array.');
    }
    if (manifest.releases.length === 0) {
      throw new Error('Manifest contains no release records.');
    }
  }

  getLatestRelease() {
    if (!this.manifest) return null;
    const latestVer = this.manifest.latest;
    return this.manifest.releases.find(r => r.version === latestVer) || this.manifest.releases[0];
  }

  getTargetConfig(chipKey) {
    const release = this.getLatestRelease();
    if (!release || !release.targets) return null;

    const normalized = chipKey?.toLowerCase().replace(/[-_]/g, '');
    for (const [key, cfg] of Object.entries(release.targets)) {
      if (key.toLowerCase().replace(/[-_]/g, '') === normalized) {
        return cfg;
      }
    }
    return null;
  }
}
