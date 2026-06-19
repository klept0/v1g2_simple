<script>
	import PageHeader from '$lib/components/PageHeader.svelte';
	import StatusAlert from '$lib/components/StatusAlert.svelte';
	import SettingsObdCard from '$lib/features/settings/SettingsObdCard.svelte';

	let message = $state(null);
</script>

<div class="page-stack">
	<PageHeader title="Integrations" subtitle="OBD connectivity and speed source status." />

	<StatusAlert {message} />

	<SettingsObdCard />

	<!-- JBV1 Integration -->
	<div class="card surface-card">
		<div class="card-body gap-4">
			<h2 class="card-title">JBV1 (GPS / Speed Display)</h2>
			<p class="copy-body">
				JBV1 is a companion app that streams speed, posted speed limit, GPS heading, satellite count,
				and GPS accuracy to v1simple over Wi-Fi. This data is displayed on the <strong>JBV1 screen</strong>
				(swipe right from the radar screen).
			</p>

			<div class="divider my-1">Requirements</div>
			<ul class="list-disc list-inside copy-body space-y-1">
				<li>Your phone must be connected to the <strong>v1simple Wi-Fi hotspot</strong>.</li>
				<li>JBV1 app must be configured to push data to the v1simple REST endpoint.</li>
				<li>Data expires after <strong>10 seconds</strong> of no updates — the JBV1 screen will show "NO JBV1 DATA" until updates resume.</li>
			</ul>

			<div class="divider my-1">Setup</div>
			<ol class="list-decimal list-inside copy-body space-y-2">
				<li>Connect your phone to the v1simple Wi-Fi network (SSID and password are shown on the <a href="/settings" class="link link-primary">Settings</a> page).</li>
				<li>In the JBV1 app, enable the <strong>REST push</strong> or <strong>HTTP export</strong> feature and set the endpoint URL to:
					<pre class="bg-base-200 rounded p-2 mt-1 text-sm overflow-x-auto">http://192.168.4.1/api/jbv1/update</pre>
				</li>
				<li>Set the push interval to <strong>1–2 seconds</strong> for smooth updates.</li>
				<li>Navigate to the JBV1 screen on the display — tap the <strong>right zone</strong> (right quarter of the screen) to advance screens.</li>
			</ol>

			<div class="divider my-1">API Reference</div>
			<p class="copy-caption">The endpoint accepts a JSON POST body:</p>
			<pre class="bg-base-200 rounded p-2 text-sm overflow-x-auto">{`POST /api/jbv1/update
Content-Type: application/json

{
  "speed": 72,
  "speedLimit": 65,
  "heading": "NW",
  "gpsAccuracy": 3,
  "satellites": 8
}`}</pre>
			<p class="copy-caption">All fields are optional — only present fields are updated. Returns <code>{`{"ok":true}`}</code> on success.</p>
		</div>
	</div>
</div>
