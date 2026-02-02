import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'

// https://vite.dev/config/
export default defineConfig({
  server: {
    allowedHosts: ['polite-bags-type.loca.lt'],
  },
  plugins: [
    tailwindcss(),
    react(),
  ],
})
