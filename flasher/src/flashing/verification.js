/**
 * verification.js
 * Browser-based cryptographic hash verification for firmware integrity.
 *
 * NOTE ON SECURITY ARCHITECTURE (Prompt Part Y):
 * - Integrity: Confirms that the binary artifact transferred across the network matches
 *   the manifest digest bit-for-bit, preventing corruption and transmission errors.
 * - Authenticity: Requires hardware-rooted digital signatures (e.g. ESP32 Secure Boot v2
 *   using RSA-3072 or ECDSA-P256 burned into Efuse). A matching SHA-256 proves integrity
 *   against the manifest, not author origin authenticity.
 */

export class FirmwareVerifier {
  /**
   * Calculate SHA-256 hex string using browser SubtleCrypto
   * @param {ArrayBuffer|Uint8Array} buffer 
   * @returns {Promise<string>} Hex-encoded lowercase SHA-256 hash
   */
  static async computeSHA256(buffer) {
    const hashBuffer = await crypto.subtle.digest('SHA-256', buffer);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    return hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
  }

  /**
   * Verify image integrity against expected SHA-256
   */
  static async verifyIntegrity(buffer, expectedHex) {
    if (!expectedHex) {
      throw new Error('No expected SHA-256 provided for verification.');
    }
    const computedHex = await this.computeSHA256(buffer);
    const match = (computedHex.toLowerCase() === expectedHex.toLowerCase());
    return {
      valid: match,
      expected: expectedHex.toLowerCase(),
      computed: computedHex.toLowerCase()
    };
  }
}
