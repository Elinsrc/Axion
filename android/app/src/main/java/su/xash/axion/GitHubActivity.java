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
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

public class GitHubActivity extends AppCompatActivity {

    public static final String COMMITS_API = "https://api.github.com/repos/Elinsrc/Axion/commits?per_page=100";
    public static final String DOWNLOAD_URL = "https://github.com/Elinsrc/Axion/releases/download/continuous/Axion-android.apk";
    public static final String REPO_URL = "https://github.com/Elinsrc/Axion";
    public static boolean sTestMode = false;
    private final Handler handler = new Handler();

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

        final String author = "Elinsrc";
        final String fullCommitHash = BuildConfig.COMMIT_HASH;
        final String commitMessage = BuildConfig.COMMIT_MESSAGE;
        final String commitUrl = REPO_URL + "/commit/" + fullCommitHash;
        final String buildDate = new SimpleDateFormat("dd.MM.yyyy HH:mm", Locale.getDefault()).format(new Date());

        ((TextView) findViewById(R.id.buildDateText)).setText(getString(R.string.build_date, buildDate));
        ((TextView) findViewById(R.id.commitHashText)).setText(getString(R.string.commit_hash, fullCommitHash));
        ((TextView) findViewById(R.id.commitMessageText)).setText(commitMessage);
        findViewById(R.id.commitClickableArea).setOnClickListener(v -> startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(commitUrl))));

        ((TextView) findViewById(R.id.authorName)).setText(author);
        loadAvatar(author, findViewById(R.id.authorAvatar));
        findViewById(R.id.authorClickArea).setOnClickListener(v -> startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse("https://github.com/" + author))));

        findViewById(R.id.repoLinkButton).setOnClickListener(v -> startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(REPO_URL))));

        if (sTestMode) {
            findViewById(R.id.testModeText).setVisibility(View.VISIBLE);
            simulateUpdate(true);
        }

        setupTestMode();
        checkUpdates(fullCommitHash);
    }

    private void setupTestMode() {
        View appIcon = findViewById(R.id.appIcon);
        Runnable testModeRunnable = () -> {
            sTestMode = true;
            findViewById(R.id.testModeText).setVisibility(View.VISIBLE);
            simulateUpdate(true);
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

    private void simulateUpdate(boolean outdated) {
        TextView statusText = findViewById(R.id.updateStatusText);
        statusText.setVisibility(View.VISIBLE);
        if (outdated) {
            statusText.setText(R.string.version_outdated);
            statusText.setTextColor(getResources().getColor(android.R.color.holo_red_light));
        } else {
            statusText.setText(R.string.up_to_date);
            statusText.setTextColor(getResources().getColor(android.R.color.holo_green_light));
        }
    }

    private void checkUpdates(String currentHash) {
        new Thread(() -> {
            try {
                URL url = new URL(COMMITS_API);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
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
                boolean outdated = true;
                if (commits.length() > 0 && commits.getJSONObject(0).getString("sha").equals(currentHash)) {
                    outdated = false;
                }

                final boolean finalOutdated = outdated;
                runOnUiThread(() -> {
                    if (!sTestMode) {
                        simulateUpdate(finalOutdated);
                    }
                });

            } catch (Exception e) {
                runOnUiThread(() -> {
                    TextView statusText = findViewById(R.id.updateStatusText);
                    if (!sTestMode) {
                        statusText.setText(R.string.update_error);
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
