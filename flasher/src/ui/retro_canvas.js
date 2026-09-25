/**
 * retro_canvas.js
 * High-performance HTML5 Canvas renderer for Space Micro-Lander (App 4).
 * Renders authentic vector retro graphics driven strictly by ESP32-S3 physics state.
 */

export class RetroCanvasRenderer {
  constructor(canvas) {
    this.canvas = canvas;
    this.ctx = canvas.getContext('2d');
    this.width = canvas.width;
    this.height = canvas.height;

    // Fixed scale from 128x64 virtual coordinates to 640x360 canvas
    this.scaleX = this.width / 128;
    this.scaleY = this.height / 64;

    this.particles = [];
    this.stars = [];
    this._initStars();

    // Latest physics state from ESP32
    this.state = {
      x: 20,
      y: 5,
      vx: 0,
      vy: 0,
      fuel: 500,
      score: 0,
      status: 0, // 0: PLAYING, 1: LANDED, 2: CRASHED
      padX: 50,
      padY: 60,
      padW: 28,
    };
  }

  _initStars() {
    this.stars = [];
    for (let i = 0; i < 40; i++) {
      this.stars.push({
        x: Math.random() * this.width,
        y: Math.random() * (this.height - 60),
        size: Math.random() * 1.5 + 0.5,
        alpha: Math.random() * 0.7 + 0.3,
        twinkleSpeed: Math.random() * 0.05 + 0.01,
      });
    }
  }

  updateState(physicsState) {
    if (!physicsState) return;

    // Detect crash transition to spawn debris particles
    if (this.state.status !== 2 && physicsState.status === 2) {
      this._spawnCrashDebris(physicsState.x * this.scaleX, physicsState.y * this.scaleY);
    }

    this.state = { ...this.state, ...physicsState };
  }

  _spawnCrashDebris(cx, cy) {
    for (let i = 0; i < 30; i++) {
      const angle = Math.random() * Math.PI * 2;
      const speed = Math.random() * 4 + 1;
      this.particles.push({
        x: cx,
        y: cy,
        vx: Math.cos(angle) * speed,
        vy: Math.sin(angle) * speed - 1,
        life: 1.0,
        decay: Math.random() * 0.03 + 0.01,
        color: Math.random() > 0.5 ? '#ff4d4d' : '#ffa500',
      });
    }
  }

  spawnThrustParticle(isThrusting) {
    if (!isThrusting || this.state.status !== 0 || this.state.fuel <= 0) return;

    const lx = (this.state.x + 3) * this.scaleX;
    const ly = (this.state.y + 5) * this.scaleY;

    for (let i = 0; i < 2; i++) {
      this.particles.push({
        x: lx + (Math.random() - 0.5) * 6,
        y: ly,
        vx: (Math.random() - 0.5) * 2,
        vy: Math.random() * 3 + 2,
        life: 0.8,
        decay: 0.08,
        color: Math.random() > 0.4 ? '#00e5ff' : '#ffffff',
      });
    }
  }

  render(isThrusting = false) {
    const ctx = this.ctx;
    ctx.clearRect(0, 0, this.width, this.height);

    // Deep space background
    const bgGrad = ctx.createLinearGradient(0, 0, 0, this.height);
    bgGrad.addColorStop(0, '#040711');
    bgGrad.addColorStop(0.8, '#0b1329');
    bgGrad.addColorStop(1, '#111a38');
    ctx.fillStyle = bgGrad;
    ctx.fillRect(0, 0, this.width, this.height);

    // Render stars
    for (const s of this.stars) {
      s.alpha += Math.sin(Date.now() * s.twinkleSpeed) * 0.02;
      ctx.fillStyle = `rgba(255, 255, 255, ${Math.max(0.2, Math.min(1.0, s.alpha))})`;
      ctx.beginPath();
      ctx.arc(s.x, s.y, s.size, 0, Math.PI * 2);
      ctx.fill();
    }

    // Render lunar surface terrain line
    const surfaceY = 60 * this.scaleY;
    ctx.strokeStyle = '#4a5b78';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(0, surfaceY);
    ctx.lineTo(this.state.padX * this.scaleX, surfaceY);
    ctx.moveTo((this.state.padX + this.state.padW) * this.scaleX, surfaceY);
    ctx.lineTo(this.width, surfaceY);
    ctx.stroke();

    // Render landing platform
    const padStartX = this.state.padX * this.scaleX;
    const padEndX = (this.state.padX + this.state.padW) * this.scaleX;

    ctx.strokeStyle = '#00f5a0';
    ctx.lineWidth = 4;
    ctx.beginPath();
    ctx.moveTo(padStartX, surfaceY);
    ctx.lineTo(padEndX, surfaceY);
    ctx.stroke();

    // Pad marker beacons
    const beaconPulse = Math.sin(Date.now() * 0.008) > 0;
    ctx.fillStyle = beaconPulse ? '#00f5a0' : '#007548';
    ctx.beginPath();
    ctx.arc(padStartX, surfaceY - 4, 3, 0, Math.PI * 2);
    ctx.arc(padEndX, surfaceY - 4, 3, 0, Math.PI * 2);
    ctx.fill();

    // Render particles
    this.spawnThrustParticle(isThrusting);
    for (let i = this.particles.length - 1; i >= 0; i--) {
      const p = this.particles[i];
      p.x += p.vx;
      p.y += p.vy;
      p.life -= p.decay;

      if (p.life <= 0) {
        this.particles.splice(i, 1);
        continue;
      }

      ctx.fillStyle = p.color;
      ctx.globalAlpha = p.life;
      ctx.beginPath();
      ctx.arc(p.x, p.y, 2.5, 0, Math.PI * 2);
      ctx.fill();
      ctx.globalAlpha = 1.0;
    }

    // Render lander sprite
    if (this.state.status !== 2) { // Not crashed
      const lx = this.state.x * this.scaleX;
      const ly = this.state.y * this.scaleY;
      const lw = 6 * this.scaleX;
      const lh = 5 * this.scaleY;

      ctx.save();
      ctx.translate(lx + lw / 2, ly + lh / 2);

      // Tilt based on horizontal speed
      const tilt = Math.max(-0.35, Math.min(0.35, this.state.vx * 0.05));
      ctx.rotate(tilt);

      // Lander body
      ctx.strokeStyle = '#ffffff';
      ctx.fillStyle = '#1c2844';
      ctx.lineWidth = 2;

      // Cabin / Capsule
      ctx.beginPath();
      ctx.moveTo(-lw * 0.3, lh * 0.2);
      ctx.lineTo(0, -lh * 0.4);
      ctx.lineTo(lw * 0.3, lh * 0.2);
      ctx.closePath();
      ctx.fill();
      ctx.stroke();

      // Lower frame
      ctx.strokeRect(-lw * 0.35, lh * 0.1, lw * 0.7, lh * 0.2);

      // Landing legs
      ctx.beginPath();
      ctx.moveTo(-lw * 0.35, lh * 0.25);
      ctx.lineTo(-lw * 0.5, lh * 0.5);
      ctx.moveTo(lw * 0.35, lh * 0.25);
      ctx.lineTo(lw * 0.5, lh * 0.5);
      ctx.stroke();

      // Thrust plume
      if (isThrusting && this.state.fuel > 0 && this.state.status === 0) {
        ctx.fillStyle = '#ff7b00';
        ctx.beginPath();
        ctx.moveTo(-lw * 0.15, lh * 0.3);
        ctx.lineTo(0, lh * 0.8 + Math.random() * 6);
        ctx.lineTo(lw * 0.15, lh * 0.3);
        ctx.closePath();
        ctx.fill();
      }

      ctx.restore();
    }

    // Status overlay text
    if (this.state.status === 1) { // LANDED
      ctx.fillStyle = '#00f5a0';
      ctx.font = 'bold 28px Outfit, sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText('MODULE LANDED SAFELY', this.width / 2, this.height / 2 - 10);
      ctx.font = '16px Inter, sans-serif';
      ctx.fillStyle = '#ffffff';
      ctx.fillText('Press [A] or [RESET] for next flight', this.width / 2, this.height / 2 + 24);
    } else if (this.state.status === 2) { // CRASHED
      ctx.fillStyle = '#ff4d4d';
      ctx.font = 'bold 28px Outfit, sans-serif';
      ctx.textAlign = 'center';
      ctx.fillText('CRITICAL IMPACT — DESTROYED', this.width / 2, this.height / 2 - 10);
      ctx.font = '16px Inter, sans-serif';
      ctx.fillStyle = '#ffffff';
      ctx.fillText('Press [A] or [RESET] to retry', this.width / 2, this.height / 2 + 24);
    }
  }
}
