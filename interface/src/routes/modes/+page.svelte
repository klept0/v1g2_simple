<script>
	import { onMount } from 'svelte';
	import { fetchWithTimeout } from '$lib/utils/poll';
	import CardSectionHead from '$lib/components/CardSectionHead.svelte';
	import PageHeader from '$lib/components/PageHeader.svelte';
	import StatusAlert from '$lib/components/StatusAlert.svelte';

	const MODE_LABELS = ['Normal', 'Quiet', 'Highway', 'Night'];
	const MODE_DESCS  = [
		'Standard alerting and brightness.',
		'Reduced volume, priority arrow, lower brightness.',
		'Increased persistence, full alerts, high visibility.',
		'Dim display, reduced volume for night driving.',
	];

	let activeMode = $state(0);
	let modes = $state([
		{ brightness: 200, voiceVolume: 75, alertPersistSec: 0, priorityArrowOnly: false },
		{ brightness: 100, voiceVolume: 40, alertPersistSec: 0, priorityArrowOnly: true  },
		{ brightness: 220, voiceVolume: 75, alertPersistSec: 3, priorityArrowOnly: false },
		{ brightness: 50,  voiceVolume: 50, alertPersistSec: 0, priorityArrowOnly: false },
	]);

	let loading  = $state(true);
	let saving   = $state(false);
	let message  = $state(null);
	let editMode = $state(null);  // index of mode being edited, or null

	// Lockout state
	let lockout = $state({ enabled: true, thresholdMph: 5, isLocked: false, speedMph: 0, speedValid: false });
	let savingLockout = $state(false);

	onMount(async () => {
		await Promise.all([fetchModes(), fetchLockout()]);
	});

	async function fetchLockout() {
		try {
			const res = await fetchWithTimeout('/api/drive/lockout');
			if (res.ok) lockout = await res.json();
		} catch (e) { /* non-fatal */ }
	}

	async function saveLockout() {
		savingLockout = true;
		message = null;
		try {
			const params = new URLSearchParams({
				enabled: lockout.enabled ? '1' : '0',
				thresholdMph: String(lockout.thresholdMph),
			});
			const res = await fetchWithTimeout('/api/drive/lockout', {
				method: 'POST',
				headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
				body: params,
			});
			if (res.ok) {
				message = { type: 'success', text: 'Safety lockout settings saved.' };
			} else {
				message = { type: 'error', text: 'Failed to save lockout settings.' };
			}
		} catch (e) {
			message = { type: 'error', text: 'Connection error.' };
		} finally {
			savingLockout = false;
		}
	}

	async function fetchModes() {
		loading = true;
		try {
			const res = await fetchWithTimeout('/api/drive/mode');
			if (res.ok) {
				const data = await res.json();
				activeMode = data.activeMode ?? 0;
				if (Array.isArray(data.modes) && data.modes.length === 4) {
					modes = data.modes;
				}
			}
		} catch (e) {
			message = { type: 'error', text: 'Could not load driving modes.' };
		} finally {
			loading = false;
		}
	}

	async function applyMode(idx) {
		if (saving) return;
		saving = true;
		message = null;
		try {
			const params = new URLSearchParams({ mode: String(idx) });
			const res = await fetchWithTimeout('/api/drive/mode', {
				method: 'POST',
				headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
				body: params,
			});
			if (res.ok) {
				activeMode = idx;
				message = { type: 'success', text: `${MODE_LABELS[idx]} mode activated.` };
			} else {
				message = { type: 'error', text: 'Failed to apply mode.' };
			}
		} catch (e) {
			message = { type: 'error', text: 'Connection error.' };
		} finally {
			saving = false;
		}
	}

	async function saveModeConfig(idx) {
		if (saving) return;
		saving = true;
		message = null;
		const cfg = modes[idx];
		const pfx = `m${idx}`;
		try {
			const params = new URLSearchParams({
				[`${pfx}bright`]:  String(cfg.brightness),
				[`${pfx}vol`]:     String(cfg.voiceVolume),
				[`${pfx}persist`]: String(cfg.alertPersistSec),
				[`${pfx}prio`]:    cfg.priorityArrowOnly ? '1' : '0',
			});
			const res = await fetchWithTimeout('/api/drive/mode', {
				method: 'POST',
				headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
				body: params,
			});
			if (res.ok) {
				editMode = null;
				message = { type: 'success', text: `${MODE_LABELS[idx]} defaults saved.` };
			} else {
				message = { type: 'error', text: 'Failed to save mode defaults.' };
			}
		} catch (e) {
			message = { type: 'error', text: 'Connection error.' };
		} finally {
			saving = false;
		}
	}
</script>

<PageHeader title="Driving Modes" />

{#if message}
	<StatusAlert type={message.type} text={message.text} onDismiss={() => (message = null)} />
{/if}

{#if loading}
	<p class="loading-text">Loading…</p>
{:else}
	<CardSectionHead title="Quick Mode" />
	<div class="mode-grid">
		{#each MODE_LABELS as label, idx}
			<button
				class="mode-card {activeMode === idx ? 'active' : ''}"
				onclick={() => applyMode(idx)}
				disabled={saving}
			>
				<span class="mode-name">{label}</span>
				<span class="mode-desc">{MODE_DESCS[idx]}</span>
			</button>
		{/each}
	</div>

	<CardSectionHead title="Mode Defaults" />
	<p class="hint">Configure the brightness, volume, and alert settings each mode applies. Changes take effect the next time that mode is activated.</p>

	{#each MODE_LABELS as label, idx}
		<div class="config-row">
			<button class="config-toggle" onclick={() => (editMode = editMode === idx ? null : idx)}>
				{label} {editMode === idx ? '▲' : '▼'}
			</button>

			{#if editMode === idx}
				<div class="config-panel">
					<label>
						Brightness <span class="val">{modes[idx].brightness}</span>
						<input type="range" min="10" max="255" bind:value={modes[idx].brightness} />
					</label>
					<label>
						Voice Volume <span class="val">{modes[idx].voiceVolume}%</span>
						<input type="range" min="0" max="100" bind:value={modes[idx].voiceVolume} />
					</label>
					<label>
						Alert Persistence <span class="val">{modes[idx].alertPersistSec}s</span>
						<input type="range" min="0" max="5" bind:value={modes[idx].alertPersistSec} />
					</label>
					<label class="checkbox-row">
						<input type="checkbox" bind:checked={modes[idx].priorityArrowOnly} />
						Priority arrow only
					</label>
					<div class="config-actions">
						<button class="btn-save" onclick={() => saveModeConfig(idx)} disabled={saving}>
							Save {label} Defaults
						</button>
						<button class="btn-cancel" onclick={() => (editMode = null)}>Cancel</button>
					</div>
				</div>
			{/if}
		</div>
	{/each}
{/if}

<CardSectionHead title="Safety Lockout" />
<p class="hint">Block configuration changes (WiFi setup, profile editing, voice settings, display settings) when the vehicle is moving above the speed threshold. Mute, profile switch, and mode switch remain available at all speeds.</p>

{#if lockout.isLocked}
	<div class="lockout-banner">🔒 Configuration locked — vehicle moving at {Math.round(lockout.speedMph)} mph</div>
{/if}

<div class="lockout-form">
	<label class="checkbox-row">
		<input type="checkbox" bind:checked={lockout.enabled} />
		Enable safety lockout
	</label>
	<label>
		Speed threshold: <strong>{lockout.thresholdMph} mph</strong>
		<input type="range" min="0" max="50" bind:value={lockout.thresholdMph} disabled={!lockout.enabled} />
	</label>
	<div class="lockout-status">
		{#if lockout.speedValid}
			Current speed: {Math.round(lockout.speedMph)} mph
		{:else}
			Speed: unavailable (OBD not connected — lockout inactive)
		{/if}
	</div>
	<button class="btn-save" onclick={saveLockout} disabled={savingLockout}>
		Save Lockout Settings
	</button>
</div>

<style>
	.loading-text {
		color: var(--color-text-muted);
		padding: 1rem 0;
	}

	.hint {
		font-size: 0.85rem;
		color: var(--color-text-muted);
		margin: 0 0 0.75rem;
	}

	.mode-grid {
		display: grid;
		grid-template-columns: 1fr 1fr;
		gap: 0.75rem;
		margin-bottom: 1.5rem;
	}

	.mode-card {
		background: var(--color-surface);
		border: 2px solid var(--color-border);
		border-radius: 8px;
		padding: 0.9rem 1rem;
		text-align: left;
		cursor: pointer;
		transition: border-color 0.15s, background 0.15s;
		display: flex;
		flex-direction: column;
		gap: 0.3rem;
	}

	.mode-card:hover:not(:disabled) {
		border-color: var(--color-accent);
	}

	.mode-card.active {
		border-color: var(--color-accent);
		background: var(--color-accent-subtle, color-mix(in srgb, var(--color-accent) 12%, transparent));
	}

	.mode-card:disabled {
		opacity: 0.6;
		cursor: not-allowed;
	}

	.mode-name {
		font-weight: 600;
		font-size: 1rem;
		color: var(--color-text);
	}

	.mode-desc {
		font-size: 0.78rem;
		color: var(--color-text-muted);
		line-height: 1.3;
	}

	.config-row {
		border: 1px solid var(--color-border);
		border-radius: 6px;
		margin-bottom: 0.5rem;
		overflow: hidden;
	}

	.config-toggle {
		width: 100%;
		background: var(--color-surface);
		border: none;
		padding: 0.7rem 1rem;
		text-align: left;
		font-weight: 600;
		font-size: 0.9rem;
		color: var(--color-text);
		cursor: pointer;
	}

	.config-panel {
		padding: 0.75rem 1rem 1rem;
		border-top: 1px solid var(--color-border);
		display: flex;
		flex-direction: column;
		gap: 0.75rem;
	}

	label {
		display: flex;
		flex-direction: column;
		gap: 0.25rem;
		font-size: 0.85rem;
		color: var(--color-text-muted);
	}

	.val {
		font-weight: 600;
		color: var(--color-text);
	}

	input[type='range'] {
		width: 100%;
		accent-color: var(--color-accent);
	}

	.checkbox-row {
		flex-direction: row;
		align-items: center;
		gap: 0.5rem;
	}

	.config-actions {
		display: flex;
		gap: 0.5rem;
		padding-top: 0.25rem;
	}

	.btn-save {
		background: var(--color-accent);
		color: #fff;
		border: none;
		border-radius: 5px;
		padding: 0.45rem 1rem;
		font-size: 0.85rem;
		cursor: pointer;
		font-weight: 600;
	}

	.btn-save:disabled {
		opacity: 0.6;
		cursor: not-allowed;
	}

	.btn-cancel {
		background: transparent;
		color: var(--color-text-muted);
		border: 1px solid var(--color-border);
		border-radius: 5px;
		padding: 0.45rem 0.9rem;
		font-size: 0.85rem;
		cursor: pointer;
	}

	.lockout-banner {
		background: #f97316;
		color: #fff;
		border-radius: 6px;
		padding: 0.6rem 1rem;
		font-weight: 600;
		font-size: 0.88rem;
		margin-bottom: 0.75rem;
	}

	.lockout-form {
		display: flex;
		flex-direction: column;
		gap: 0.85rem;
		padding: 0.75rem 0 1.25rem;
	}

	.lockout-status {
		font-size: 0.8rem;
		color: var(--color-text-muted);
	}
</style>
