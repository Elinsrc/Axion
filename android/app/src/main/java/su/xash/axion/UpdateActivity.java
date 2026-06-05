package su.xash.axion;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.MotionEvent;
import android.view.View;
import android.webkit.WebSettings;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.ContextCompat;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.DataSource;
import com.bumptech.glide.load.engine.GlideException;
import com.bumptech.glide.request.RequestListener;
import com.bumptech.glide.request.target.Target;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class UpdateActivity extends AppCompatActivity {

    public static final String COMMITS_API = "https://api.github.com/repos/Elinsrc/Axion/commits?per_page=1";
    public static final String DOWNLOAD_URL = "https://github.com/Elinsrc/Axion/releases/tag/continuous";
    public static final String REPO_URL = "https://github.com/Elinsrc/Axion";
    public static boolean sTestMode = false;

    private static final String AUTHOR = "Elinsrc";

    private final Handler handler = new Handler(Looper.getMainLooper());
    private TextView statusText;

    public interface UpdateCallback {
        void onUpdateCheckResult(boolean success, boolean outdated, @Nullable String hash, @Nullable String msg);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        toolbar.setNavigationOnClickListener(v -> finish());

        statusText = findViewById(R.id.updateStatusText);
        String commitHash = BuildConfig.COMMIT_HASH;

        ((TextView) findViewById(R.id.buildDateText)).setText(getString(R.string.build_date, BuildConfig.BUILD_DATE));
        ((TextView) findViewById(R.id.commitHashText)).setText(getString(R.string.commit_hash, commitHash));
        ((TextView) findViewById(R.id.commitMessageText)).setText(BuildConfig.COMMIT_MESSAGE);
        ((TextView) findViewById(R.id.authorName)).setText(AUTHOR);

        TextView repoLink = findViewById(R.id.repoLinkText);
        repoLink.setText(REPO_URL);
        repoLink.setOnClickListener(v -> openUrl(REPO_URL));

        findViewById(R.id.commitClickableArea).setOnClickListener(v -> openUrl(REPO_URL + "/commit/" + commitHash));
        findViewById(R.id.githubButton).setOnClickListener(v -> openUrl("https://github.com/" + AUTHOR));
        findViewById(R.id.telegramButton).setOnClickListener(v -> openUrl("https://t.me/" + AUTHOR));

        loadAvatar(AUTHOR, findViewById(R.id.authorAvatar));

        if (sTestMode) applyTestModeUi();

        checkUpdates(this, commitHash, (success, outdated, hash, msg) -> {
            if (!sTestMode) setStatusOutdated(outdated);
        });

        Runnable enableTestMode = () -> {
            sTestMode = true;
            applyTestModeUi();
        };

        findViewById(R.id.appIcon).setOnTouchListener((v, event) -> {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    handler.postDelayed(enableTestMode, 5000);
                    break;
                case MotionEvent.ACTION_UP:
                    handler.removeCallbacks(enableTestMode);
                    v.performClick();
                    break;
                case MotionEvent.ACTION_CANCEL:
                    handler.removeCallbacks(enableTestMode);
                    break;
            }
            return true;
        });
    }

    private void applyTestModeUi() {
        findViewById(R.id.testModeText).setVisibility(View.VISIBLE);
        setStatusOutdated(true);
    }

    private void setStatusOutdated(boolean outdated) {
        if (outdated) {
            statusText.setText(R.string.version_outdated);
            statusText.setTextColor(ContextCompat.getColor(this, android.R.color.holo_red_light));
            statusText.setVisibility(View.VISIBLE);
        } else {
            statusText.setVisibility(View.GONE);
        }
    }

    public static void checkUpdates(Activity activity, String currentHash, UpdateCallback callback) {
        new Thread(() -> {
            try {
                JSONObject latest = fetchLatestCommit(activity);
                if (latest == null) throw new Exception("Empty response");

                String hash = latest.getString("sha");
                String msg = latest.getJSONObject("commit").getString("message");
                boolean outdated = !hash.equals(currentHash);

                activity.runOnUiThread(() -> callback.onUpdateCheckResult(true, outdated, hash, msg));
            } catch (Exception e) {
                activity.runOnUiThread(() -> callback.onUpdateCheckResult(false, false, null, null));
            }
        }).start();
    }

    public static void showUpdateInfo(Activity activity, String hash, String message) {
        TextView status = activity.findViewById(R.id.updateStatusText);
        status.setText(R.string.update_available);
        status.setTextColor(activity.getColor(android.R.color.holo_red_light));
        status.setVisibility(View.VISIBLE);

        activity.findViewById(R.id.latestCommitArea).setVisibility(View.VISIBLE);
        activity.findViewById(R.id.updateButton).setVisibility(View.VISIBLE);

        ((TextView) activity.findViewById(R.id.latestCommitHash)).setText(activity.getString(R.string.commit_hash, hash));
        ((TextView) activity.findViewById(R.id.latestCommitMessage)).setText(message);

        activity.findViewById(R.id.latestCommitArea).setOnClickListener(v ->
                activity.startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(REPO_URL + "/commit/" + hash))));
    }

    private static JSONObject fetchLatestCommit(Context context) throws Exception {
        HttpURLConnection conn = (HttpURLConnection) new URL(COMMITS_API).openConnection();
        conn.setRequestProperty("User-Agent", WebSettings.getDefaultUserAgent(context));
        conn.setConnectTimeout(5000);

        try (BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream()))) {
            StringBuilder response = new StringBuilder();
            String line;
            while ((line = reader.readLine()) != null) response.append(line);
            JSONArray array = new JSONArray(response.toString());
            return array.length() > 0 ? array.getJSONObject(0) : null;
        } finally {
            conn.disconnect();
        }
    }

    private void loadAvatar(String username, ImageView imageView) {
        Glide.with(this)
                .load("https://github.com/" + username + ".png")
                .circleCrop()
                .listener(new RequestListener<>() {
                    @Override
                    public boolean onLoadFailed(@Nullable GlideException e, Object model, @NonNull Target<Drawable> target, boolean isFirst) {
                        imageView.setVisibility(View.GONE);
                        return false;
                    }

                    @Override
                    public boolean onResourceReady(@NonNull Drawable resource, @NonNull Object model, Target<Drawable> target, @NonNull DataSource source, boolean isFirst) {
                        imageView.setVisibility(View.VISIBLE);
                        return false;
                    }
                })
                .into(imageView);
    }

    private void openUrl(String url) {
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
    }
}