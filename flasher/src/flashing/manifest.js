/**
 * manifest.js
 * Resolves the latest public SliverOS firmware release from GitHub.
 * Release assets are immutable build outputs; GitHub supplies SHA-256 digests
 * for integrity validation and browser_download_url for direct download.
 */

const RELEASE_API = 'https://api.github.com/repos/bishwajit5788/SliverOS/releases/latest';

export class ManifestManager {
  constructor(manifestUrl = './firmware/releases.json') {
    this.manifestUrl = manifestUrl;
    this.manifest = null;
  }

  async load() {
    try {
      const response = await fetch(RELEASE_API, {
        headers: { Accept: 'application/vnd.github+json' },
        cache: 'no-store'
      });
      if (!response.ok) throw new Error(`GitHub release API returned HTTP ${response.status}`);

      const release = await response.json();
      this.manifest = this.fromRelease(release);
      this.validate(this.manifest);
      return this.manifest;
    } catch (releaseError) {
      /* Keep a local manifest path for self-hosted/offline development. */
      try {
        const response = await fetch(this.manifestUrl, { cache: 'no-store' });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        this.manifest = await response.json();
        this.validate(this.manifest);
        return this.manifest;
      } catch (localError) {
        throw new Error(`No published SliverOS firmware release is available. ${releaseError.message}`);
      }
    }
  }

  fromRelease(release) {
    const assets = new Map((release.assets || []).map(asset => [asset.name, asset]));
    const required = [
      ['bootloader.bin', 0x000000],
      ['partition-table.bin', 0x008000],
      ['ota_data_initial.bin', 0x00F000],
      ['phy_init_data.bin', 0x011000],
      ['microkernel-esp32.bin', 0x020000]
    ];

    const images = [];
    for (const [name, offset] of required) {
      const asset = assets.get(name);
      if (!asset) {
        if (name === 'phy_init_data.bin' || name === 'ota_data_initial.bin') continue;
        throw new Error(`Published release is incomplete: missing ${name}`);
      }
      const digest = asset.digest?.replace(/^sha256:/i, '');
      if (!digest) throw new Error(`Published asset ${name} has no SHA-256 digest.`);
      images.push({
        name,
        filename: asset.browser_download_url,
        url: asset.browser_download_url,
        offset,
        sha256: digest,
        is_fixture: false
      });
    }

    return {
      product: 'SliverOS',
      latest: release.tag_name,
      releases: [{
        version: release.tag_name,
        targets: {
          esp32s3: {
            chip: 'ESP32-S3',
            version: release.tag_name,
            flash_mode: 'dio',
            flash_freq: '40m',
            flash_size: 'detect',
            erase_all: false,
            images
          }
        }
      }]
    };
  }

  validate(manifest) {
    if (!manifest?.product || !Array.isArray(manifest.releases) || manifest.releases.length === 0) {
      throw new Error('Invalid firmware manifest.');
    }
    for (const release of manifest.releases) {
      if (!release.targets?.esp32s3?.images?.length) {
        throw new Error('Firmware manifest has no ESP32-S3 image set.');
      }
    }
  }

  getLatestRelease() {
    if (!this.manifest) return null;
    return this.manifest.releases.find(r => r.version === this.manifest.latest) || this.manifest.releases[0];
  }

  getTargetConfig(chipKey) {
    const release = this.getLatestRelease();
    if (!release?.targets) return null;
    const normalized = chipKey?.toLowerCase().replace(/[-_]/g, '');
    for (const [key, config] of Object.entries(release.targets)) {
      if (key.toLowerCase().replace(/[-_]/g, '') === normalized) return config;
    }
    return null;
  }
}
