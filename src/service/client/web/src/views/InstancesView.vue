<script setup>
import { onMounted, onBeforeUnmount } from 'vue'
import InstanceList from '../components/instance/InstanceList.vue'
import { useInstanceStore } from '../stores/instance.js'

const instanceStore = useInstanceStore()

onMounted(() => {
  instanceStore.bootstrap()
})

onBeforeUnmount(() => {
  instanceStore.stopPolling()
})
</script>

<template>
  <InstanceList />
</template>
