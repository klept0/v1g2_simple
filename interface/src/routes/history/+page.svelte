<script>
	import { onMount } from 'svelte';
	import { fetchWithTimeout } from '$lib/utils/poll';
	import CardSectionHead from '$lib/components/CardSectionHead.svelte';
	import PageHeader from '$lib/components/PageHeader.svelte';
	import StatusAlert from '$lib/components/StatusAlert.svelte';

	let encounters = $state([]);
	let loading    = $state(true);
	let message    = $state(null);
	let busy       = $state(false);

	const BAND_COLORS = {
		Ka: '#f97316',
		K:  '#3b82f6',
		X:  '#8b5cf6',
		Laser: '#ef4444',
		None: '#6b7280',
	};

	function bandColor(band) {
		return BAND_COLORS[band] ?? BAND_COLORS.None;
	}

	function formatTs(tsMs) {
		if (!tsMs) return '—';
		const d = new Date(Number(tsMs));
		return d.toLocaleString();
	}

	function formatFreq(freq) {
		if (!freq) return '—';
		return (freq / 1000).toFixed(3) + ' GHz';
	}

	onMount(async () => {
		await fetchHistory();
	});

	async function fetchHistory() {
		loading = true;
		try {
			const res = await fetchWithTimeout('/api/history');
			if (res.ok) {
				encounters = await res.json();
			} else {
				message = { type: 'error', text: 'Failed to load history.' };
			}
		} catch (e) {
			message = { type: 'error', text: 'Connection error.' };
		} finally {
			loading = false;
		}
	}

	async function clearHistory() {
		if (!confirm('Clear all encounter history? This cannot be undone.')) return;
		busy = true;
		message = null;
		try {
			const res = await fetchWithTimeout('/api/history/clear', { method: 'POST' });
			if (res.ok) {
				encounters = [];
				message = { type: 'success', text: 'History cleared.' };
			} else {
				message = { type: 'error', text: 'Clear failed.' };
			}
		} catch (e) {
			message = { type: 'error', text: 'Connection error.' };
		} finally {
			busy = false;
		}
	}

	async function toggleFalse(entry) {
		busy = true;
		const newVal = !entry.false;
		try {
			const params = new URLSearchParams({ id: String(entry.id), value: newVal ? '1' : '0' });
			const res = await fetchWithTimeout('/api/history/mark-false', {
				method: 'POST',
				headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
				body: params,
			});
			if (res.ok) {
				encounters = encounters.map(e => e.id === entry.id ? { ...e, false: newVal } : e);
			} else {
				message = { type: 'error', text: 'Failed to update entry.' };
			}
		} catch (e) {
			message = { type: 'error', text: 'Connection error.' };
		} finally {
			busy = false;
		}
	}

	function exportCsv() {
		window.location.href = '/api/history/export.csv';
	}
</script>

<PageHeader title="Encounter History" />

{#if message}
	<StatusAlert type={message.type} text={message.text} onDismiss={() => (message = null)} />
{/if}

<div class="toolbar">
	<span class="count">{encounters.length} encounter{encounters.length !== 1 ? 's' : ''}</span>
	<div class="actions">
		<button class="btn-export" onclick={exportCsv} disabled={encounters.length === 0}>
			Export CSV
		</button>
		<button class="btn-clear" onclick={clearHistory} disabled={busy || encounters.length === 0}>
			Clear All
		</button>
	</div>
</div>

{#if loading}
	<p class="loading-text">Loading…</p>
{:else if encounters.length === 0}
	<p class="empty-text">No encounters recorded yet.</p>
{:else}
	<div class="table-wrap">
		<table>
			<thead>
				<tr>
					<th>Time</th>
					<th>Band</th>
					<th>Frequency</th>
					<th>Direction</th>
					<th>Bogeys</th>
					<th>Signal</th>
					<th>Speed</th>
					<th>Slot</th>
					<th>Muted</th>
					<th>False</th>
				</tr>
			</thead>
			<tbody>
				{#each encounters as enc (enc.id)}
					<tr class:false-alert={enc.false} class:muted-row={enc.muted}>
						<td class="ts">{formatTs(enc.ts)}</td>
						<td>
							<span class="band-pill" style="background:{bandColor(enc.band)}">
								{enc.band}
							</span>
						</td>
						<td>{formatFreq(enc.freq)}</td>
						<td>{enc.dir}</td>
						<td class="center">{enc.bogeys}</td>
						<td class="center">
							<span class="bars">{enc.bars}/6</span>
						</td>
						<td class="center">{enc.speed > 0 ? enc.speed + ' mph' : '—'}</td>
						<td>{enc.slot}</td>
						<td class="center">{enc.muted ? '🔇' : ''}</td>
						<td class="center">
							<button
								class="false-btn {enc.false ? 'is-false' : ''}"
								onclick={() => toggleFalse(enc)}
								disabled={busy}
								title={enc.false ? 'Unmark false alert' : 'Mark as false alert'}
							>
								{enc.false ? '✓' : '○'}
							</button>
						</td>
					</tr>
				{/each}
			</tbody>
		</table>
	</div>
{/if}

<style>
	.toolbar {
		display: flex;
		align-items: center;
		justify-content: space-between;
		margin-bottom: 1rem;
		gap: 0.5rem;
	}

	.count {
		font-size: 0.85rem;
		color: var(--color-text-muted);
	}

	.actions {
		display: flex;
		gap: 0.5rem;
	}

	.btn-export, .btn-clear {
		border: 1px solid var(--color-border);
		border-radius: 5px;
		padding: 0.35rem 0.8rem;
		font-size: 0.8rem;
		cursor: pointer;
		background: var(--color-surface);
		color: var(--color-text);
	}

	.btn-clear {
		color: #ef4444;
		border-color: #ef4444;
	}

	.btn-export:disabled, .btn-clear:disabled {
		opacity: 0.4;
		cursor: not-allowed;
	}

	.loading-text, .empty-text {
		color: var(--color-text-muted);
		padding: 2rem 0;
		text-align: center;
	}

	.table-wrap {
		overflow-x: auto;
	}

	table {
		width: 100%;
		border-collapse: collapse;
		font-size: 0.82rem;
	}

	th {
		text-align: left;
		padding: 0.4rem 0.6rem;
		border-bottom: 2px solid var(--color-border);
		color: var(--color-text-muted);
		font-weight: 600;
		white-space: nowrap;
	}

	td {
		padding: 0.35rem 0.6rem;
		border-bottom: 1px solid var(--color-border);
		vertical-align: middle;
	}

	.center {
		text-align: center;
	}

	.ts {
		white-space: nowrap;
		font-size: 0.78rem;
	}

	.band-pill {
		display: inline-block;
		padding: 0.15rem 0.5rem;
		border-radius: 20px;
		color: #fff;
		font-weight: 700;
		font-size: 0.75rem;
	}

	.bars {
		font-variant-numeric: tabular-nums;
	}

	tr.false-alert td {
		opacity: 0.45;
	}

	tr.muted-row .band-pill {
		filter: saturate(0.4);
	}

	.false-btn {
		background: none;
		border: 1px solid var(--color-border);
		border-radius: 4px;
		width: 26px;
		height: 22px;
		cursor: pointer;
		font-size: 0.7rem;
		color: var(--color-text-muted);
		display: inline-flex;
		align-items: center;
		justify-content: center;
	}

	.false-btn.is-false {
		background: #ef4444;
		border-color: #ef4444;
		color: #fff;
	}

	.false-btn:disabled {
		opacity: 0.4;
		cursor: not-allowed;
	}
</style>
