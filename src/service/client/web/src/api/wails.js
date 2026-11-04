export function api() {
  const app = window?.go?.main?.App
  if (!app) {
    throw new Error('tebox backend is not ready')
  }
  return app
}
