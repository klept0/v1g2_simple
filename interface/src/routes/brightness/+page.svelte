<script>
	import { onMount } from 'svelte';
	import { fetchWithTimeout } from '$lib/utils/poll';
	import CardSectionHead from '$lib/components/CardSectionHead.svelte';
	import PageHeader from '$lib/components/PageHeader.svelte';
	import StatusAlert from '$lib/components/StatusAlert.svelte';
	import ToggleSetting from '$lib/components/ToggleSetting.svelte';

	const ENDPOINT = '/api/display/brightness';

	let cfg = $state({
		enabled: true,
		brtDay: 200,
		brtNight: 80,
		brtIdle: 50,
		brtAlert: 255,
		brtMute: 120,
		brtIdleSec: 30,
	});

	let loading = $state(true);
	let saving  = $state(false);
	let message = $state(null);

	onMount(async () => {
		try {
			const res = await fetchWithTimeout(ENDPOINT);
			if (res.ok) cfg = await res.json();
		} catch (e) {
			message = { type: 'error', text: 'Could not load brightness settings.' };
		} finally {
			loading = false;
		}
	});

	async function save() {
		saving = true;
		message = null;
		try {
			const params = new URLSearchParams({
				enabled:   cfg.enabled ? '1' : '0',
				brtDay:    String(cfg.brtDay),
				brtNight:  String(cfg.brtNight),
				brtIdle:   String(cfg.brtIdle),
				brtAlert:  String(cfg.brtAlert),
				brtMute:   String(cfg.brtMute),
				brtIdleSec: String(cfg.brtIdleSec),
			});
			const res = await fetchWithTimeout(ENDPOINT, {
				method: 'POST',
				headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
				body: params,
			});
			if (res.ok) {
				message = { type: 'success', text: 'Brightness settings saved.' };
			} else {
				const data = await res.json().catch(() => ({}));
				message = { type: 'error', text: data.error || 'Failed to save.' };
			}
		} catch (e) {
			message = { type: 'error', text: 'Connection error.' };
		} finally {
			saving = false;
		}
	}
</script>

<PageHeader title="Smart Brightness" />
<StatusAlert {message} />

{#if loading}
	<p class="loading-text">Loading…</p>
{:else}
	<ToggleSetting
		label="Enable Smart Brightness"
		description="Automatically adjusts brightness based on alerts, mute state, and idle time."
		bind:checked={cfg.enabled}
	/>

	<CardSectionHead title="Brightness Levels" />

	<label>
		Day / Normal: <strong>{cfg.brtDay}</strong>
		<input type="range" min="10" max="255" bind:value={cfg.brtDay} disabled={!cfg.enabled} />
	</label>

	<label>
		Night: <strong>{cfg.brtNight}</strong>
		<input type="range" min="10" max="255" bind:value={cfg.brtNight} disabled={!cfg.enabled} />
	</label>

	<label>
		Alert Boost: <strong>{cfg.brtAlert}</strong>
		<input type="range" min="10" max="255" bind:value={cfg.brtAlert} disabled={!cfg.enabled} />
	</label>

	<label>
		Muted Alert: <strong>{cfg.brtMute}</strong>
		<input type="range" min="10" max="255" bind:value={cfg.brtMute} disabled={!cfg.enabled} />
	</label>

	<CardSectionHead title="Idle Dim" />

	<label>
		Idle Brightness: <strong>{cfg.brtIdle}</strong>
		<input type="range" min="10" max="255" bind:value={cfg.brtIdle} disabled={!cfg.enabled} />
	</label>

	<label>
		Idle Timeout: <strong>{cfg.brtIdleSec === 0 ? 'Disabled' : cfg.brtIdleSec + 's'}</strong>
		<input type="range" min="0" max="300" step="5" bind:value={cfg.brtIdleSec} disabled={!cfg.enabled} />
		<span class="hint">0 = no idle dim. Alerts, touch, or button press wake the display.</span>
	</label>

	<button class="btn-save" onclick={save} disabled={saving || !cfg.enabled}>
		Save
	</button>
{/if}

<style>
	.loading-text {
		color: var(--color-text-muted);
		padding: 1rem 0;
	}

	label {
		display: flex;
		flex-direction: column;
		gap: 0.25rem;
		font-size: 0.85rem;
		color: var(--color-text-muted);
		margin-bottom: 0.75rem;
	}

	input[type="range"] {
		width: 100%;
	}

	.hint {
		font-size: 0.75rem;
		color: var(--color-text-muted);
	}

	.btn-save {
		margin-top: 1rem;
		background: var(--color-primary);
		color: #fff;
		border: none;
		border-radius: 6px;
		padding: 0.5rem 1.1rem;
		font-size: 0.85rem;
		cursor: pointer;
	}

	.btn-save:disabled {
		opacity: 0.5;
		cursor: not-allowed;
	}
</style>
