<script setup>
import AppHeader from './AppHeader.vue'
import NoticeToast from '../ui/NoticeToast.vue'
</script>

<template>
  <div class="shell">
    <AppHeader />
    <main class="main">
      <slot />
    </main>
    <NoticeToast />
  </div>
</template>

<style scoped>
.shell {
  height: 100%;
  display: grid;
  grid-template-rows: auto 1fr;
  overflow: hidden;
}

.main {
  min-height: 0;
  overflow: hidden;
}
</style>
