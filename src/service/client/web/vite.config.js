import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],
  clearScreen: false,
  server: {
    strictPort: true,
  },
  build: {
    outDir: '../cmd/server/web/dist',
    emptyOutDir: true,
  },
})
