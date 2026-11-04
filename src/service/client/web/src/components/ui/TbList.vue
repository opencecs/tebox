<script setup>
defineProps({
  empty: {
    type: Boolean,
    default: false,
  },
  loading: {
    type: Boolean,
    default: false,
  },
  columns: {
    type: Array,
    default: () => [],
  },
})

function gridStyle(columns) {
  return {
    gridTemplateColumns: columns.map((c) => c.width || '1fr').join(' '),
  }
}
</script>

<template>
  <div class="tb-list" :class="{ empty: empty && !loading, loading }">
    <div v-if="columns.length" class="tb-list-head" :style="gridStyle(columns)">
      <div
        v-for="col in columns"
        :key="col.key"
        class="tb-list-head-cell"
        :class="{ end: col.align === 'end' }"
      >
        {{ col.label }}
      </div>
    </div>

    <div v-if="loading" class="tb-list-loading">
      <span class="tb-spinner" aria-hidden="true" />
      <span>Loading…</span>
    </div>
    <div v-else-if="empty" class="tb-list-empty">
      <slot name="empty">No items</slot>
    </div>
    <div v-else class="tb-list-body">
      <slot />
    </div>
  </div>
</template>

<style scoped>
.tb-list {
  display: flex;
  flex-direction: column;
  height: 100%;
  min-height: 0;
  background: var(--panel);
  overflow: auto;
}

.tb-list-head {
  display: grid;
  align-items: center;
  min-height: 40px;
  background: #fafafa;
  color: var(--muted);
  font-size: 12px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.02em;
  flex-shrink: 0;
  position: sticky;
  top: 0;
  z-index: 1;
}

.tb-list-head-cell {
  padding: 0 12px;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.tb-list-head-cell.end {
  text-align: right;
}

.tb-list-loading,
.tb-list-empty {
  display: flex;
  flex: 1;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 10px;
  color: var(--muted);
  font-size: 13px;
  text-align: center;
}

.tb-list-empty {
  gap: 4px;
}

.tb-list-empty :deep(strong) {
  color: var(--ink);
  font-size: 14px;
  font-weight: 600;
}

.tb-spinner {
  width: 28px;
  height: 28px;
  border: 2px solid color-mix(in srgb, var(--accent) 22%, var(--line));
  border-top-color: var(--accent);
  border-radius: 50%;
  animation: tb-spin 0.7s linear infinite;
}

.tb-list-body {
  display: flex;
  flex-direction: column;
  flex: 1;
}

@keyframes tb-spin {
  to {
    transform: rotate(360deg);
  }
}
</style>
