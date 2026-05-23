package su.xash.axion;

import android.content.Intent;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.view.MotionEvent;
import android.view.View;
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
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class GitHubActivity extends AppCompatActivity {

    public static final String COMMITS_API = "https://api.github.com/repos/Elinsrc/Axion/commits?per_page=100";
    public static final String DOWNLOAD_URL = "https://github.com/Elinsrc/Axion/releases/tag/continuous";
    public static final String REPO_URL = "https://github.com/Elinsrc/Axion";
    public static boolean sTestMode = false;
    private final Handler handler = new Handler();
    private TextView statusText;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_about);

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        if (getSupportActionBar() != null) {
            getSupportActionBar().setDisplayHomeAsUpEnabled(true);
        }
        toolbar.setNavigationOnClickListener(v -> finish());

        statusText = findViewById(R.id.updateStatusText);
        final String author = "Elinsrc";
        final String fullCommitHash = BuildConfig.COMMIT_HASH;

        ((TextView) findViewById(R.id.buildDateText)).setText(getString(R.string.build_date, BuildConfig.BUILD_DATE));
        ((TextView) findViewById(R.id.commitHashText)).setText(getString(R.string.commit_hash, fullCommitHash));
        ((TextView) findViewById(R.id.commitMessageText)).setText(BuildConfig.COMMIT_MESSAGE);

        TextView repoLinkText = findViewById(R.id.repoLinkText);
        repoLinkText.setText(REPO_URL);
        repoLinkText.setOnClickListener(v -> openUrl(REPO_URL));

        findViewById(R.id.commitClickableArea).setOnClickListener(v -> openUrl(REPO_URL + "/commit/" + fullCommitHash));
        findViewById(R.id.githubButton).setOnClickListener(v -> openUrl("https://github.com/" + author));
        findViewById(R.id.telegramButton).setOnClickListener(v -> openUrl("https://t.me/" + author));

        ((TextView) findViewById(R.id.authorName)).setText(author);
        loadAvatar(author, findViewById(R.id.authorAvatar));

        if (sTestMode) {
            findViewById(R.id.testModeText).setVisibility(View.VISIBLE);
            showStatus(R.string.version_outdated);
        }

        setupTestMode();
        checkUpdates(fullCommitHash);
    }

    private void openUrl(String url) {
        startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url)));
    }

    private void setupTestMode() {
        View appIcon = findViewById(R.id.appIcon);
        Runnable testModeRunnable = () -> {
            sTestMode = true;
            findViewById(R.id.testModeText).setVisibility(View.VISIBLE);
            showStatus(R.string.version_outdated);
        };

        appIcon.setOnTouchListener((v, event) -> {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    handler.postDelayed(testModeRunnable, 5000);
                    break;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    handler.removeCallbacks(testModeRunnable);
                    v.performClick();
                    break;
            }
            return true;
        });
    }

    private void showStatus(int stringResId) {
        if (stringResId == 0) {
            statusText.setVisibility(View.GONE);
        } else {
            statusText.setText(stringResId);
            statusText.setTextColor(ContextCompat.getColor(this, android.R.color.holo_red_light));
            statusText.setVisibility(View.VISIBLE);
        }
    }

    private void checkUpdates(String currentHash) {
        new Thread(() -> {
            try {
                HttpURLConnection conn = (HttpURLConnection) new URL(COMMITS_API).openConnection();
                conn.setRequestMethod("GET");
                conn.setRequestProperty("User-Agent", "Axion-App");

                BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream()));
                StringBuilder response = new StringBuilder();
                String inputLine;
                while ((inputLine = reader.readLine()) != null) {
                    response.append(inputLine);
                }
                reader.close();

                JSONArray commits = new JSONArray(response.toString());
                boolean current = commits.length() > 0 && commits.getJSONObject(0).getString("sha").equals(currentHash);

                runOnUiThread(() -> {
                    if (!sTestMode) {
                        showStatus(current ? 0 : R.string.version_outdated);
                    }
                });

            } catch (Exception e) {
                runOnUiThread(() -> {
                    if (!sTestMode) {
                        showStatus(R.string.update_error);
                    }
                });
            }
        }).start();
    }

    private void loadAvatar(String username, final ImageView imageView) {
        Glide.with(this)
                .load("https://github.com/" + username + ".png")
                .circleCrop()
                .listener(new RequestListener<>() {
                    @Override
                    public boolean onLoadFailed(@Nullable GlideException e, Object model, @NonNull Target<Drawable> target, boolean isFirstResource) {
                        imageView.setVisibility(View.GONE);
                        return false;
                    }

                    @Override
                    public boolean onResourceReady(@NonNull Drawable resource, @NonNull Object model, Target<Drawable> target, @NonNull DataSource dataSource, boolean isFirstResource) {
                        imageView.setVisibility(View.VISIBLE);
                        return false;
                    }
                })
                .into(imageView);
    }
}