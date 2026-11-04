import { createRouter, createWebHashHistory } from 'vue-router'
import InstancesView from '../views/InstancesView.vue'

const routes = [
  {
    path: '/',
    name: 'instances',
    component: InstancesView,
  },
]

export const router = createRouter({
  history: createWebHashHistory(),
  routes,
})
