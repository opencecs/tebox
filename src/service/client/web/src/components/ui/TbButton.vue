<script setup>
import { ref } from 'vue'

defineProps({
  variant: {
    type: String,
    default: 'default', // default | primary | danger | ghost
  },
  disabled: {
    type: Boolean,
    default: false,
  },
  icon: {
    type: Boolean,
    default: false,
  },
  title: {
    type: String,
    default: '',
  },
})

const ripples = ref([])
let rippleSeq = 0

function spawnRipple(e) {
  const el = e.currentTarget
  if (el.disabled) return
  const rect = el.getBoundingClientRect()
  const size = Math.max(rect.width, rect.height) * 2.2
  const id = ++rippleSeq
  const point = e.touches?.[0] || e
  ripples.value.push({
    id,
    x: point.clientX - rect.left - size / 2,
    y: point.clientY - rect.top - size / 2,
    size,
  })
  window.setTimeout(() => {
    ripples.value = ripples.value.filter((r) => r.id !== id)
  }, 600)
}
</script>

<template>
  <button
    type="button"
    class="tb-btn"
    :class="[variant, { icon }]"
    :disabled="disabled"
    :title="title"
    @mousedown="spawnRipple"
    @touchstart.passive="spawnRipple"
  >
    <span class="tb-btn-label"><slot /></span>
    <span
      v-for="r in ripples"
      :key="r.id"
      class="tb-ripple"
      :style="{
        left: `${r.x}px`,
        top: `${r.y}px`,
        width: `${r.size}px`,
        height: `${r.size}px`,
      }"
    />
  </button>
</template>

<style scoped>
.tb-btn {
  appearance: none;
  position: relative;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  margin: 0;
  overflow: hidden;
  isolation: isolate;
  border: 1px solid var(--line);
  border-radius: var(--radius);
  background: var(--panel);
  color: var(--ink);
  padding: 8px 12px;
  font-size: 13px;
  font-weight: 600;
  line-height: 1;
  cursor: pointer;
  user-select: none;
  -webkit-tap-highlight-color: transparent;
  transition:
    background var(--dur-fast) var(--ease-out),
    border-color var(--dur-fast) var(--ease-out),
    color var(--dur-fast) var(--ease-out),
    transform var(--dur-fast) var(--ease-out),
    opacity var(--dur-fast) var(--ease-out);
}

.tb-btn-label {
  position: relative;
  z-index: 1;
  display: inline-flex;
  align-items: center;
  gap: 6px;
  pointer-events: none;
  transition: transform var(--dur-fast) var(--ease-out);
}

.tb-ripple {
  position: absolute;
  border-radius: 50%;
  background: rgba(0, 0, 0, 0.22);
  pointer-events: none;
  transform: scale(0);
  animation: tb-ripple 0.55s cubic-bezier(0, 0, 0.2, 1) forwards;
  z-index: 0;
}

@keyframes tb-ripple {
  0% {
    transform: scale(0);
    opacity: 0.45;
  }
  100% {
    transform: scale(1);
    opacity: 0;
  }
}

.tb-btn.icon {
  width: 32px;
  height: 32px;
  padding: 0;
}

.tb-btn:hover:not(:disabled) {
  background: var(--row-hover);
}

.tb-btn:active:not(:disabled) {
  transform: scale(0.96);
}

.tb-btn.primary {
  background: var(--accent);
  border-color: var(--accent);
  color: var(--accent-ink);
}

.tb-btn.primary:hover:not(:disabled) {
  background: var(--accent-hover);
  border-color: var(--accent-hover);
  color: var(--accent-ink);
}

.tb-btn.primary .tb-ripple {
  background: rgba(255, 255, 255, 0.45);
}

.tb-btn.danger {
  color: var(--danger);
  border-color: color-mix(in srgb, var(--danger) 30%, var(--line));
}

.tb-btn.danger:hover:not(:disabled) {
  background: color-mix(in srgb, var(--danger) 8%, var(--panel));
  border-color: var(--danger);
}

.tb-btn.danger.armed {
  background: var(--danger);
  border-color: var(--danger);
  color: #fff;
}

.tb-btn.danger.armed:hover:not(:disabled) {
  background: #b91c1c;
  border-color: #b91c1c;
  color: #fff;
}

.tb-btn.danger.armed .tb-ripple {
  background: rgba(255, 255, 255, 0.45);
}

.tb-btn.ghost {
  background: transparent;
  border-color: transparent;
  color: var(--ink);
}

.tb-btn.ghost:hover:not(:disabled) {
  background: var(--row-hover);
  color: var(--ink);
}

.tb-btn:disabled {
  opacity: 0.45;
  cursor: default;
  transform: none;
}
</style>
