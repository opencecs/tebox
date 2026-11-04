<script setup>
import { Plus } from 'lucide-vue-next'
import { useInstanceStore } from '../../stores/instance.js'
import TbButton from '../ui/TbButton.vue'

const instanceStore = useInstanceStore()
</script>

<template>
  <header class="top">
    <h1>OpenCECS TeBox</h1>
    <TbButton variant="primary" @click="instanceStore.create()">
      <Plus :size="16" :stroke-width="2.5" />
      New
    </TbButton>
  </header>
</template>

<style scoped>
.top {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  min-height: 52px;
  background: var(--panel);
}

.top h1 {
  margin: 0 0 0 12px;
  font-size: 16px;
  font-weight: 700;
  color: #000;
  line-height: 1.2;
}

.top :deep(.tb-btn) {
  margin-right: 8px;
}
</style>
