<script setup>
import { storeToRefs } from 'pinia'
import { CircleAlert, CircleCheck, Info, X } from 'lucide-vue-next'
import { useNoticeStore } from '../../stores/notice.js'

const notice = useNoticeStore()
const { items } = storeToRefs(notice)

const icons = {
  info: Info,
  success: CircleCheck,
  warning: CircleAlert,
  error: CircleAlert,
}
</script>

<template>
  <div class="notice-stack" aria-live="polite">
    <TransitionGroup name="notice">
      <div
        v-for="item in items"
        :key="item.id"
        class="notice"
        :class="item.type"
        role="status"
      >
        <component
          :is="icons[item.type] || icons.info"
          class="notice-icon"
          :size="22"
          :stroke-width="2.25"
        />
        <div class="notice-body">
          <div class="notice-title">{{ item.title }}</div>
          <div v-if="item.description" class="notice-desc">{{ item.description }}</div>
        </div>
        <button
          type="button"
          class="notice-close"
          aria-label="Close"
          @click="notice.close(item.id)"
        >
          <X :size="14" :stroke-width="2.25" />
        </button>
      </div>
    </TransitionGroup>
  </div>
</template>

<style scoped>
.notice-stack {
  position: fixed;
  left: 16px;
  bottom: 16px;
  z-index: 1010;
  display: flex;
  flex-direction: column-reverse;
  gap: 16px;
  width: min(384px, calc(100vw - 32px));
  pointer-events: none;
}

.notice {
  pointer-events: auto;
  display: flex;
  align-items: flex-start;
  gap: 12px;
  padding: 16px 20px;
  background: #fff;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
  line-height: 1.5714;
}

.notice-icon {
  flex-shrink: 0;
  margin-top: 1px;
}

.notice.info .notice-icon {
  color: #1677ff;
}

.notice.success .notice-icon {
  color: #52c41a;
}

.notice.warning .notice-icon {
  color: #faad14;
}

.notice.error .notice-icon {
  color: #ff4d4f;
}

.notice-body {
  flex: 1;
  min-width: 0;
}

.notice-title {
  font-size: 16px;
  font-weight: 600;
  color: rgba(0, 0, 0, 0.88);
  line-height: 1.5;
}

.notice-desc {
  margin-top: 4px;
  font-size: 14px;
  color: rgba(0, 0, 0, 0.65);
  word-break: break-word;
}

.notice-close {
  appearance: none;
  flex-shrink: 0;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 22px;
  height: 22px;
  margin: -2px -6px 0 0;
  padding: 0;
  border: 0;
  border-radius: 4px;
  background: transparent;
  color: rgba(0, 0, 0, 0.45);
  cursor: pointer;
}

.notice-close:hover {
  color: rgba(0, 0, 0, 0.88);
  background: rgba(0, 0, 0, 0.04);
}

.notice-enter-active {
  transition:
    opacity 0.24s cubic-bezier(0.23, 1, 0.32, 1),
    transform 0.24s cubic-bezier(0.23, 1, 0.32, 1);
}

.notice-leave-active {
  position: absolute;
  left: 0;
  right: 0;
  transition:
    opacity 0.2s cubic-bezier(0.755, 0.05, 0.855, 0.06),
    transform 0.2s cubic-bezier(0.755, 0.05, 0.855, 0.06);
}

.notice-enter-from {
  opacity: 0;
  transform: translateX(-100%);
}

.notice-leave-to {
  opacity: 0;
  transform: translateX(-100%);
  height: 0;
  margin: 0;
  padding-top: 0;
  padding-bottom: 0;
}

.notice-move {
  transition: transform 0.24s cubic-bezier(0.23, 1, 0.32, 1);
}
</style>
