<script setup>
defineProps({
  active: {
    type: Boolean,
    default: false,
  },
  columns: {
    type: Array,
    default: () => [],
  },
})

defineEmits(['click'])

function gridStyle(columns) {
  if (!columns.length) return {}
  return {
    gridTemplateColumns: columns.map((c) => c.width || '1fr').join(' '),
  }
}
</script>

<template>
  <div
    class="tb-list-item"
    :class="{ active, grid: columns.length > 0 }"
    :style="gridStyle(columns)"
    role="button"
    tabindex="0"
    @click="$emit('click')"
    @keydown.enter.prevent="$emit('click')"
  >
    <template v-if="columns.length">
      <slot />
    </template>
    <template v-else>
      <div class="tb-list-item-main">
        <slot />
      </div>
      <div class="tb-list-item-actions" @click.stop>
        <slot name="actions" />
      </div>
    </template>
  </div>
</template>

<style scoped>
.tb-list-item {
  display: grid;
  grid-template-columns: 1fr auto;
  align-items: center;
  gap: 12px;
  min-height: 48px;
  background: var(--panel);
  cursor: pointer;
  outline: none;
}

.tb-list-item.grid {
  gap: 0;
}

.tb-list-item:hover {
  background: var(--row-hover);
}

.tb-list-item.active {
  background: var(--row-active);
}

.tb-list-item-main {
  min-width: 0;
  display: flex;
  flex-direction: column;
  gap: 2px;
  margin-left: 12px;
}

.tb-list-item-actions {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-right: 8px;
}

.tb-list-item :deep(.cell) {
  min-width: 0;
  padding: 0 12px;
  font-size: 13px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.tb-list-item :deep(.cell.end) {
  text-align: right;
}

.tb-list-item :deep(.cell.actions) {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  gap: 6px;
}
</style>
