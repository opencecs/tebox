<script setup>
import { ref, computed } from 'vue'
import { Play, Square, Trash2, Settings } from 'lucide-vue-next'
import { storeToRefs } from 'pinia'
import { INSTANCE_COLUMNS, useInstanceStore } from '../../stores/instance.js'
import TbButton from '../ui/TbButton.vue'
import TbDialog from '../ui/TbDialog.vue'
import TbListItem from '../ui/TbListItem.vue'

const props = defineProps({
  instance: { type: Object, required: true },
})

const instanceStore = useInstanceStore()
const { selectedId } = storeToRefs(instanceStore)
const columns = INSTANCE_COLUMNS

const showDelete = ref(false)
const showSettings = ref(false)
const busy = ref(false)
const profile = ref({ ...(props.instance.profile || {}) })

const isRunning = computed(() => !!props.instance.running)
const active = computed(() => props.instance.id === selectedId.value)

async function onToggle() {
  if (busy.value) return
  busy.value = true
  try {
    if (isRunning.value) {
      await instanceStore.stop(props.instance.id)
    } else {
      await instanceStore.start(props.instance.id)
    }
  } catch {
    // notice handled in store
  } finally {
    busy.value = false
  }
}

function onDelete() {
  instanceStore.select(props.instance.id)
  showDelete.value = true
}

async function confirmDelete() {
  if (busy.value) return
  busy.value = true
  try {
    await instanceStore.remove(props.instance.id)
    showDelete.value = false
  } catch {
    // notice handled in store
  } finally {
    busy.value = false
  }
}

async function saveSettings() {
  if (busy.value) return
  busy.value = true
  try {
    await instanceStore.configure(props.instance.id, profile.value)
    showSettings.value = false
  } catch {
    // notice handled in store
  } finally {
    busy.value = false
  }
}
</script>

<template>
  <div class="instance-row">
    <TbListItem
      :active="active"
      :columns="columns"
      @click="instanceStore.select(instance.id)"
    >
      <div class="cell">{{ instance.name }}</div>
      <div class="cell">
        <span class="state" :class="{ on: isRunning }">{{ isRunning ? 'Running' : 'Stopped' }}</span>
      </div>
      <div class="cell">{{ instance.variant }}</div>
      <div class="cell">{{ instance.adbPort || '—' }}</div>
      <div class="cell actions end" @click.stop>
        <TbButton
          icon
          variant="quiet"
          :disabled="busy"
          title="Device settings"
          @click="showSettings = true"
        >
          <Settings :size="16" :stroke-width="2.25" />
        </TbButton>
        <TbButton
          icon
          variant="primary"
          :disabled="busy"
          :title="isRunning ? 'Stop' : 'Start'"
          @click="onToggle"
        >
          <Square v-if="isRunning" :size="16" :stroke-width="2.25" />
          <Play v-else :size="16" :stroke-width="2.25" />
        </TbButton>
        <TbButton
          icon
          variant="danger"
          :disabled="busy"
          title="Delete"
          @click="onDelete"
        >
          <Trash2 :size="16" :stroke-width="2.25" />
        </TbButton>
      </div>
    </TbListItem>

    <TbDialog
      v-model:open="showDelete"
      title="Delete instance"
      :message="`Delete “${instance.name}”? This cannot be undone.`"
      confirm-text="Delete"
      cancel-text="Cancel"
      danger
      :busy="busy"
      @confirm="confirmDelete"
    />
    <TbDialog
      v-model:open="showSettings"
      title="Device settings"
      confirm-text="Apply"
      cancel-text="Cancel"
      :busy="busy"
      @confirm="saveSettings"
    >
      <div class="settings-grid">
        <label>Latitude<input v-model.number="profile.latitude" type="number" step="0.000001" /></label>
        <label>Longitude<input v-model.number="profile.longitude" type="number" step="0.000001" /></label>
        <label>Country<input v-model="profile.country" maxlength="2" /></label>
        <label>Operator<input v-model="profile.operatorName" /></label>
        <label>PLMN<input v-model="profile.operatorNumeric" /></label>
        <label>Cell ID<input v-model="profile.cellId" /></label>
        <label>Phone number<input v-model="profile.phoneNumber" /></label>
        <label class="check"><input v-model="profile.root" type="checkbox" /> Request ADB root</label>
      </div>
    </TbDialog>
  </div>
</template>

<style scoped>
.instance-row {
  display: block;
}

.state.on {
  color: var(--ok);
  font-weight: 600;
}
.settings-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; }
.settings-grid label { display: grid; gap: 4px; color: var(--muted); font-size: 12px; }
.settings-grid input:not([type="checkbox"]) { min-width: 0; padding: 7px 8px; }
.settings-grid .check { display: flex; align-items: center; gap: 7px; grid-column: 1 / -1; }
</style>
