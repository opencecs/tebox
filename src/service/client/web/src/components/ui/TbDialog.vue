<script setup>
import { watch, onBeforeUnmount } from 'vue'
import TbButton from './TbButton.vue'

const props = defineProps({
  open: { type: Boolean, default: false },
  title: { type: String, default: 'Confirm' },
  message: { type: String, default: '' },
  confirmText: { type: String, default: 'Confirm' },
  cancelText: { type: String, default: 'Cancel' },
  danger: { type: Boolean, default: false },
  busy: { type: Boolean, default: false },
})

const emit = defineEmits(['confirm', 'cancel', 'update:open'])

function close() {
  if (props.busy) return
  emit('update:open', false)
  emit('cancel')
}

function confirm() {
  if (props.busy) return
  emit('confirm')
}

function onKey(e) {
  if (!props.open) return
  if (e.key === 'Escape') close()
  if (e.key === 'Enter') confirm()
}

watch(
  () => props.open,
  (open) => {
    if (open) window.addEventListener('keydown', onKey)
    else window.removeEventListener('keydown', onKey)
  },
)

onBeforeUnmount(() => {
  window.removeEventListener('keydown', onKey)
})
</script>

<template>
  <Teleport to="body">
    <Transition name="tb-dialog">
      <div v-if="open" class="tb-dialog-root" @mousedown.self="close">
        <div class="tb-dialog" role="dialog" aria-modal="true" :aria-label="title">
          <h2 class="tb-dialog-title">{{ title }}</h2>
          <p v-if="message" class="tb-dialog-msg">{{ message }}</p>
          <div v-if="$slots.default" class="tb-dialog-body"><slot /></div>
          <div class="tb-dialog-actions">
            <TbButton :disabled="busy" @click="close">{{ cancelText }}</TbButton>
            <TbButton
              :variant="danger ? 'danger' : 'primary'"
              :class="{ armed: danger }"
              :disabled="busy"
              @click="confirm"
            >
              {{ confirmText }}
            </TbButton>
          </div>
        </div>
      </div>
    </Transition>
  </Teleport>
</template>

<style scoped>
.tb-dialog-root {
  position: fixed;
  inset: 0;
  z-index: 100;
  display: grid;
  place-items: center;
  background: rgba(0, 0, 0, 0.35);
}

.tb-dialog {
  width: min(360px, calc(100vw - 32px));
  background: #fff;
  border-radius: var(--radius);
  box-shadow: 0 16px 48px rgba(0, 0, 0, 0.2);
  transform-origin: center center;
}

.tb-dialog-title {
  margin: 0;
  padding: 16px 16px 0;
  font-size: 15px;
  font-weight: 700;
  color: #000;
}

.tb-dialog-msg {
  margin: 8px 0 0;
  padding: 0 16px;
  font-size: 13px;
  color: var(--muted);
  line-height: 1.45;
}

.tb-dialog-actions {
  display: flex;
  justify-content: flex-end;
  gap: 8px;
  padding: 16px;
}

.tb-dialog-enter-active,
.tb-dialog-leave-active {
  transition: opacity var(--dur) var(--ease-out);
}

.tb-dialog-enter-active .tb-dialog,
.tb-dialog-leave-active .tb-dialog {
  transition:
    transform var(--dur-slow) var(--ease-spring),
    opacity var(--dur) var(--ease-out);
}

.tb-dialog-enter-from,
.tb-dialog-leave-to {
  opacity: 0;
}

.tb-dialog-enter-from .tb-dialog {
  opacity: 0;
  transform: translateY(12px) scale(0.92);
}

.tb-dialog-leave-to .tb-dialog {
  opacity: 0;
  transform: translateY(8px) scale(0.96);
}
</style>
