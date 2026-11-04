import { defineStore } from 'pinia'
import { ref } from 'vue'

let seq = 0

function toText(value) {
  if (value == null) return ''
  if (typeof value === 'string') return value
  if (value instanceof Error) return value.message || String(value)
  if (typeof value === 'object') {
    if (value.description != null) return String(value.description)
    if (value.message != null) return String(value.message)
  }
  return String(value)
}

function normalize(input, type = 'info') {
  if (
    input &&
    typeof input === 'object' &&
    !Array.isArray(input) &&
    !(input instanceof Error) &&
    (input.description != null || input.message != null || input.title != null)
  ) {
    return {
      type: input.type || type,
      title: input.title || defaultTitle(input.type || type),
      description: toText(input.description ?? input.message),
      duration: input.duration ?? 4.5,
    }
  }
  return {
    type,
    title: defaultTitle(type),
    description: toText(input),
    duration: 4.5,
  }
}

function defaultTitle(type) {
  switch (type) {
    case 'success':
      return 'Success'
    case 'error':
      return 'Error'
    case 'warning':
      return 'Warning'
    default:
      return 'Info'
  }
}

export const useNoticeStore = defineStore('notice', () => {
  const items = ref([])
  const timers = new Map()

  function push(input, type = 'info') {
    const payload = normalize(input, type)
    if (!payload.description) return

    const id = ++seq
    items.value = [...items.value, { id, ...payload }]

    const ms = Number(payload.duration) * 1000
    if (ms > 0) {
      const timer = setTimeout(() => close(id), ms)
      timers.set(id, timer)
    }
    return id
  }

  function success(input) {
    return push(input, 'success')
  }

  function error(input) {
    return push(input, 'error')
  }

  function warning(input) {
    return push(input, 'warning')
  }

  function info(input) {
    return push(input, 'info')
  }

  function close(id) {
    const timer = timers.get(id)
    if (timer) {
      clearTimeout(timer)
      timers.delete(id)
    }
    items.value = items.value.filter((item) => item.id !== id)
  }

  function dismiss() {
    for (const timer of timers.values()) clearTimeout(timer)
    timers.clear()
    items.value = []
  }

  return {
    items,
    push,
    info,
    success,
    error,
    warning,
    close,
    dismiss,
  }
})
