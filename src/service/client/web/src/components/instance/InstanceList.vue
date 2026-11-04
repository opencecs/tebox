<script setup>
import { storeToRefs } from 'pinia'
import { INSTANCE_COLUMNS, useInstanceStore } from '../../stores/instance.js'
import TbList from '../ui/TbList.vue'
import InstanceRow from './InstanceRow.vue'

const instanceStore = useInstanceStore()
const { instances, loading, isEmpty } = storeToRefs(instanceStore)
const columns = INSTANCE_COLUMNS
</script>

<template>
  <TbList :empty="isEmpty" :loading="loading" :columns="columns">
    <template #empty>
      <strong>No instances</strong>
      <span>Create one to launch QEMU</span>
    </template>
    <InstanceRow
      v-for="item in instances"
      :key="item.id"
      :instance="item"
    />
  </TbList>
</template>
