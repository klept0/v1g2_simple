<script>
	import { onMount } from 'svelte';
	import { fetchWithTimeout } from '$lib/utils/poll';
	import PageHeader from '$lib/components/PageHeader.svelte';
	import StatusAlert from '$lib/components/StatusAlert.svelte';

	const ENDPOINT = '/api/setup/wizard';

	const STEPS = [
		{
			title: 'Welcome',
			icon: '📡',
			description: 'This wizard helps you configure your Valentine One Gen2 companion display. You can skip any step and run this wizard again at any time from the Settings page.',
			detail: null,
		},
		{
			title: 'Pair Valentine One',
			icon: '🔵',
			description: 'Connect your display to the Valentine One Gen2 via Bluetooth.',
			detail: 'Go to the Devices page and tap "Scan" to find your V1 Gen2. Once connected, the V1 icon in the status bar will turn green.',
		},
		{
			title: 'Configure WiFi',
			icon: '📶',
			description: 'Optionally connect the display to your home network for remote access.',
			detail: 'Go to Settings → WiFi to configure a client connection. While driving, the device creates its own WiFi hotspot automatically.',
		},
		{
			title: 'Configure OBD',
			icon: '🚗',
			description: 'Connect an OBD-II Bluetooth adapter for real speed data and safety lockout.',
			detail: 'Go to the Devices page and scan for your OBD adapter. Speed data enables the driving safety lockout feature, which prevents accidental setting changes while moving.',
		},
		{
			title: 'Select Driving Mode',
			icon: '🛣️',
			description: 'Choose a preset that matches your typical driving style.',
			detail: 'Visit the Driving Modes page to select Normal, Quiet, Highway, or Night mode. Each preset adjusts brightness, alert volume, and persistence. You can switch modes any time.',
		},
		{
			title: 'Brightness Calibration',
			icon: '☀️',
			description: 'Adjust display brightness levels to suit your environment.',
			detail: 'Open the Brightness page to set Day, Night, Idle, Alert, and Muted brightness levels. The Smart Brightness Engine adjusts automatically based on alert state and touch activity.',
		},
		{
			title: 'Speaker Test',
			icon: '🔊',
			description: 'Verify alert audio is working correctly.',
			detail: 'Go to the Audio page and use the Test buttons to play alert tones for each band. Adjust the voice volume and enable or disable startup/shutdown chimes to your preference.',
		},
		{
			title: 'Backup Configuration',
			icon: '💾',
			description: 'Save your settings to an SD card or cloud backup for safekeeping.',
			detail: 'Visit the Auto-Push page to configure automatic backups, or use the Profiles page to manually export your configuration. Backups survive firmware updates.',
		},
	];

	const TOTAL = STEPS.length;

	let step    = $state(0);   // 0-indexed current step (0 = welcome)
	let done    = $state(false);
	let loading = $state(true);
	let saving  = $state(false);
	let message = $state(null);

	onMount(async () => {
		try {
			const res = await fetchWithTimeout(ENDPOINT);
			if (res.ok) {
				const data = await res.json();
				done = data.done ?? false;
				step = data.step ?? 0;
				// If wizard was previously completed but user navigated here, restart from 0
				if (done) step = 0;
			}
		} catch (e) {
			message = { type: 'error', text: 'Could not load wizard state.' };
		} finally {
			loading = false;
		}
	});

	async function saveState(newStep, newDone) {
		saving = true;
		try {
			const res = await fetchWithTimeout(ENDPOINT, {
				method: 'POST',
				headers: { 'Content-Type': 'application/json' },
				body: JSON.stringify({ step: newStep, done: newDone }),
			});
			if (!res.ok) throw new Error('Save failed');
		} catch (e) {
			message = { type: 'error', text: 'Could not save wizard state.' };
		} finally {
			saving = false;
		}
	}

	function goNext() {
		if (step < TOTAL - 1) {
			step++;
			saveState(step, false);
		} else {
			completeWizard();
		}
	}

	function goPrev() {
		if (step > 0) {
			step--;
			saveState(step, false);
		}
	}

	function completeWizard() {
		done = true;
		saveState(step, true);
	}

	function skipAll() {
		done = true;
		step = 0;
		saveState(0, true);
	}

	function restartWizard() {
		done = false;
		step = 0;
		saveState(0, false);
	}

	$derived: const progress = ((step + 1) / TOTAL) * 100;
</script>

<PageHeader title="Setup Wizard" />

{#if message}
	<StatusAlert type={message.type} text={message.text} onDismiss={() => (message = null)} />
{/if}

{#if loading}
	<div class="card"><p class="loading-text">Loading wizard…</p></div>
{:else if done}
	<!-- Completion screen -->
	<div class="card wizard-card complete-card">
		<div class="complete-icon">✅</div>
		<h2 class="complete-title">Setup Complete</h2>
		<p class="complete-body">Your V1 companion display is configured and ready to use. You can revisit any settings page at any time.</p>
		<div class="complete-actions">
			<a href="/" class="btn btn-primary">Go to Dashboard</a>
			<button class="btn btn-ghost" onclick={restartWizard}>Run Wizard Again</button>
		</div>
	</div>
{:else}
	<!-- Step progress bar -->
	<div class="progress-bar-wrap">
		<div class="progress-bar-track">
			<div class="progress-bar-fill" style="width: {progress}%"></div>
		</div>
		<span class="progress-label">Step {step + 1} of {TOTAL}</span>
	</div>

	<!-- Step card -->
	<div class="card wizard-card">
		<div class="step-icon">{STEPS[step].icon}</div>
		<h2 class="step-title">{STEPS[step].title}</h2>
		<p class="step-description">{STEPS[step].description}</p>
		{#if STEPS[step].detail}
			<div class="step-detail">
				<p>{STEPS[step].detail}</p>
			</div>
		{/if}
	</div>

	<!-- Step dots -->
	<div class="step-dots">
		{#each STEPS as _, i}
			<button
				class="dot"
				class:active={i === step}
				class:visited={i < step}
				aria-label="Go to step {i + 1}"
				onclick={() => { step = i; saveState(i, false); }}
			></button>
		{/each}
	</div>

	<!-- Navigation -->
	<div class="wizard-nav">
		<button class="btn btn-ghost" onclick={goPrev} disabled={step === 0 || saving}>
			← Back
		</button>
		<button class="btn btn-ghost skip-btn" onclick={skipAll} disabled={saving}>
			Skip All
		</button>
		{#if step < TOTAL - 1}
			<button class="btn btn-primary" onclick={goNext} disabled={saving}>
				Next →
			</button>
		{:else}
			<button class="btn btn-primary" onclick={completeWizard} disabled={saving}>
				Finish ✓
			</button>
		{/if}
	</div>
{/if}

<style>
	.wizard-card {
		text-align: center;
		padding: 2rem 1.5rem;
	}

	.step-icon,
	.complete-icon {
		font-size: 3rem;
		margin-bottom: 0.75rem;
	}

	.step-title,
	.complete-title {
		font-size: 1.4rem;
		font-weight: 700;
		margin: 0 0 0.75rem;
	}

	.step-description,
	.complete-body {
		color: var(--text-secondary);
		line-height: 1.5;
		margin: 0 0 1rem;
	}

	.step-detail {
		background: var(--surface-alt, rgba(255,255,255,0.05));
		border: 1px solid var(--border);
		border-radius: 6px;
		padding: 0.75rem 1rem;
		text-align: left;
		font-size: 0.875rem;
		color: var(--text-secondary);
		line-height: 1.5;
	}

	.step-detail p {
		margin: 0;
	}

	.progress-bar-wrap {
		display: flex;
		align-items: center;
		gap: 0.75rem;
		margin-bottom: 1rem;
	}

	.progress-bar-track {
		flex: 1;
		height: 6px;
		background: var(--border);
		border-radius: 3px;
		overflow: hidden;
	}

	.progress-bar-fill {
		height: 100%;
		background: var(--accent, #3b82f6);
		border-radius: 3px;
		transition: width 0.3s ease;
	}

	.progress-label {
		font-size: 0.8rem;
		color: var(--text-secondary);
		white-space: nowrap;
	}

	.step-dots {
		display: flex;
		justify-content: center;
		gap: 0.5rem;
		margin: 1rem 0;
	}

	.dot {
		width: 10px;
		height: 10px;
		border-radius: 50%;
		border: none;
		background: var(--border);
		cursor: pointer;
		padding: 0;
		transition: background 0.2s;
	}

	.dot.visited {
		background: var(--accent-dim, #1e40af);
	}

	.dot.active {
		background: var(--accent, #3b82f6);
		transform: scale(1.3);
	}

	.wizard-nav {
		display: flex;
		justify-content: space-between;
		align-items: center;
		gap: 0.5rem;
		margin-top: 0.5rem;
	}

	.skip-btn {
		font-size: 0.8rem;
		opacity: 0.6;
	}

	.complete-actions {
		display: flex;
		flex-direction: column;
		gap: 0.75rem;
		align-items: center;
		margin-top: 1.5rem;
	}

	.loading-text {
		text-align: center;
		color: var(--text-secondary);
	}

	.btn {
		padding: 0.5rem 1.25rem;
		border-radius: 6px;
		font-size: 0.9rem;
		font-weight: 600;
		cursor: pointer;
		border: none;
		transition: opacity 0.15s;
	}

	.btn:disabled {
		opacity: 0.4;
		cursor: default;
	}

	.btn-primary {
		background: var(--accent, #3b82f6);
		color: #fff;
	}

	.btn-ghost {
		background: transparent;
		color: var(--text-secondary);
		border: 1px solid var(--border);
	}

	.btn-ghost:hover:not(:disabled) {
		background: var(--surface-alt, rgba(255,255,255,0.05));
	}
</style>
