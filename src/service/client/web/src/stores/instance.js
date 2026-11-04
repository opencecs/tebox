import { defineStore } from 'pinia'
import { computed, ref } from 'vue'
import { api } from '../api/wails.js'
import { useNoticeStore } from './notice.js'

export const INSTANCE_COLUMNS = [
  { key: 'name', label: 'Name', width: '1.4fr' },
  { key: 'status', label: 'Status', width: '0.8fr' },
  { key: 'variant', label: 'Variant', width: '1fr' },
  { key: 'adb', label: 'ADB', width: '0.7fr' },
  { key: 'actions', label: 'Actions', width: '100px', align: 'end' },
]

export const useInstanceStore = defineStore('instance', () => {
  const instances = ref([])
  const selectedId = ref(null)
  const loading = ref(true)
  let pollTimer = null
  let loadedOnce = false

  const selected = computed(() =>
    instances.value.find((item) => item.id === selectedId.value) || null,
  )

  const isEmpty = computed(() => instances.value.length === 0)

  function select(id) {
    selectedId.value = id
  }

  async function refresh() {
    const notice = useNoticeStore()
    const started = Date.now()
    if (!loadedOnce) loading.value = true
    try {
      instances.value = await api().ListInstances()
      if (selectedId.value && !instances.value.some((i) => i.id === selectedId.value)) {
        selectedId.value = null
      }
    } catch (err) {
      notice.error(err)
    } finally {
      if (!loadedOnce) {
        const wait = 450 - (Date.now() - started)
        if (wait > 0) await new Promise((r) => setTimeout(r, wait))
      }
      loadedOnce = true
      loading.value = false
    }
  }

  async function create() {
    const notice = useNoticeStore()
    try {
      const item = await api().CreateInstance()
      selectedId.value = item.id
      notice.success('Created')
      await refresh()
      return item
    } catch (err) {
      notice.error(err)
      throw err
    }
  }

  async function start(id) {
    const notice = useNoticeStore()
    select(id)
    try {
      await api().StartInstance(id)
      notice.info('Starting…')
      await refresh()
    } catch (err) {
      notice.error(err)
      throw err
    }
  }

  async function stop(id) {
    const notice = useNoticeStore()
    select(id)
    try {
      await api().StopInstance(id)
      notice.info('Stopping…')
      await refresh()
    } catch (err) {
      notice.error(err)
      throw err
    }
  }

  async function remove(id) {
    const notice = useNoticeStore()
    select(id)
    try {
      await api().DeleteInstance(id)
      notice.success('Deleted')
      await refresh()
    } catch (err) {
      notice.error(err)
      throw err
    }
  }

  async function configure(id, profile) {
    const notice = useNoticeStore()
    try {
      await api().ConfigureInstance(id, profile)
      notice.success('Device settings applied')
      await refresh()
    } catch (err) {
      notice.error(err)
      throw err
    }
  }

  function startPolling(intervalMs = 1000) {
    stopPolling()
    pollTimer = setInterval(() => {
      refresh()
    }, intervalMs)
  }

  function stopPolling() {
    if (pollTimer) {
      clearInterval(pollTimer)
      pollTimer = null
    }
  }

  async function bootstrap() {
    await refresh()
    startPolling()
  }

  return {
    instances,
    selectedId,
    selected,
    loading,
    isEmpty,
    select,
    refresh,
    create,
    start,
    stop,
    remove,
    configure,
    bootstrap,
    startPolling,
    stopPolling,
  }
})
