//
//  Copyright (c) 2019 Rally Tactical Systems, Inc.
//  All rights reserved.
//

package com.rallytac.engageandroid;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.Bundle;
import android.text.Html;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.widget.PopupMenu;
import androidx.core.content.ContextCompat;

import com.google.android.material.floatingactionbutton.FloatingActionButton;
import com.rallytac.engage.engine.Engine;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.UUID;

public class CertStoreListActivity extends AppCompatActivity
{
    private static String TAG = CertStoreListActivity.class.getSimpleName();

    private ArrayList<EngageCertStore> _stores;
    private CertStoreListAdapter _adapter;
    private Intent _resultIntent = new Intent();
    private String _activeCertStoreFileName;
    private String _activeMissionCertStoreId = null;
    private EngageCertStore _importedStore = null;

    private class CertStoreListAdapter extends ArrayAdapter<EngageCertStore>
    {
        private Context _ctx;
        private int _resId;

        public CertStoreListAdapter(Context ctx, int resId, ArrayList<EngageCertStore> list)
        {
            super(ctx, resId, list);
            _ctx = ctx;
            _resId = resId;
        }

        @NonNull
        @Override
        public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent)
        {
            LayoutInflater inflator = LayoutInflater.from(_ctx);
            convertView = inflator.inflate(_resId, parent, false);

            final EngageCertStore item = getItem(position);

            ((TextView)convertView.findViewById(R.id.tvCertStoreName)).setText(item.getDisplayName());
            ((TextView)convertView.findViewById(R.id.tvDescription)).setText(item.getDescription());

            convertView.setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    viewCertStore(item);
                }
            });

            convertView.findViewById(R.id.ivDeleteCertStore).setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    confirmDeleteCertStore(item);
                }
            });

            if (_activeCertStoreFileName.compareTo(item.getFileName()) == 0 || item.idMatches(_activeMissionCertStoreId) )
            {
                ((ImageView) convertView.findViewById(R.id.ivActiveCertStoreIndicator))
                        .setImageDrawable(ContextCompat.getDrawable(CertStoreListActivity.this,
                                R.drawable.ic_radio_button_selected));
            }
            else
            {
                ((ImageView) convertView.findViewById(R.id.ivActiveCertStoreIndicator))
                        .setImageDrawable(ContextCompat.getDrawable(CertStoreListActivity.this,
                                R.drawable.ic_radio_button_unselected));
            }

            convertView.findViewById(R.id.ivActiveCertStoreIndicator).setOnClickListener(new View.OnClickListener()
            {
                @Override
                public void onClick(View v)
                {
                    activateCertStoreAndFinish(item);
                }
            });

            if(item.getFileName().contains(Constants.INTERNAL_DEFAULT_CERTSTORE_FN) || item.getFileName().contains(Constants.RTS_FACTORY_CERTSTORE_FN))
            {
                convertView.findViewById(R.id.ivDeleteCertStore).setVisibility(View.INVISIBLE);
            }

            return convertView;
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_certstore_list);

        FloatingActionButton fabAdd = findViewById(R.id.fabAdd);
        fabAdd.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View view)
            {
                //promptToAddCertStore();
                selectFileForImport();
            }
        });

        _activeCertStoreFileName = Globals.getSharedPreferences().getString(PreferenceKeys.USER_CERT_STORE_FILE_NAME, "");
        ActiveConfiguration ac = Globals.getEngageApplication().getActiveConfiguration();
        if(ac != null)
        {
            _activeMissionCertStoreId = ac.getMissionCertStoreId();
        }

        loadStores(Globals.getEngageApplication().getCertStoreCacheDir(), true);

        _adapter = new CertStoreListAdapter(this, R.layout.certstore_list_entry, _stores);
        ListView lv = findViewById(R.id.lvCertStores);
        lv.setAdapter(_adapter);

        setupActionBar();
    }

    private void setupActionBar()
    {
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null)
        {
            actionBar.setDisplayHomeAsUpEnabled(true);
        }
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item)
    {
        int id = item.getItemId();
        if (id == android.R.id.home)
        {
            onBackPressed();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent)
    {
        Globals.getLogger().d(TAG, "onActivityResult");//NON-NLS

        if(resultCode == RESULT_OK)
        {
            if(requestCode == Constants.PICK_CERTSTORE_FILE_REQUEST_CODE)
            {
                importCertStoreFromUri(intent.getData());
            }
        }
    }

    private void loadStores(String sourceDirectory, boolean includeDefault)
    {
        _stores = new ArrayList<>();

        try
        {
            File dir = new File(sourceDirectory);
            File[] allContents = dir.listFiles();
            if (allContents != null)
            {
                for (File file : allContents)
                {
                    EngageCertStore cs = EngageCertStore.loadStoreFrom(file.getAbsolutePath());
                    if (cs != null)
                    {
                        if( (!cs.isAppDefault()) || (cs.isAppDefault() && includeDefault) )
                        {
                            _stores.add(cs);
                        }
                    }
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    private void promptToAddCertStore()
    {
        PopupMenu popup = new PopupMenu(this, findViewById(R.id.fabAdd));

        MenuInflater inflater = popup.getMenuInflater();
        inflater.inflate(R.menu.certstore_list_activity_menu, popup.getMenu());

        popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener()
        {
            @Override
            public boolean onMenuItemClick(MenuItem item)
            {
                final int menuId = item.getItemId();

                if ( (menuId == R.id.action_certstore_add_scan) ||
                        (menuId == R.id.action_certstore_add_load_file) )
                {
                    // Clear any left-over password
                    Utils.setInboundCertStorePassword(null);

                    LayoutInflater layoutInflater = LayoutInflater.from(CertStoreListActivity.this);
                    View promptView = layoutInflater.inflate(R.layout.certstore_load_password_dialog, null);
                    AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(CertStoreListActivity.this);
                    alertDialogBuilder.setView(promptView);

                    final EditText editText = promptView.findViewById(R.id.etPassword);

                    alertDialogBuilder.setCancelable(false)
                            .setPositiveButton(R.string.mission_load_continue_button, new DialogInterface.OnClickListener()
                            {
                                public void onClick(DialogInterface dialog, int id)
                                {
                                    // Save the password for later
                                    Utils.setInboundCertStorePassword(editText.getText().toString());

                                    if(menuId == R.id.action_certstore_add_scan)
                                    {
                                        Globals.getEngageApplication().scanQrCode(CertStoreListActivity.this, getString(R.string.scan_qr_code), findViewById(R.id.fabAdd), getString(R.string.select_qr_code_file), Constants.OFFLINE_ACTIVATION_CODE_REQUEST_CODE);
                                        //Globals.getEngageApplication().initiateScanOfAQrCode(MissionListActivity.this, getString(R.string.scan_qr_code));
                                    }
                                    else
                                    {
                                        selectFileForImport();
                                    }
                                }
                            })
                            .setNegativeButton(R.string.cancel,
                                    new DialogInterface.OnClickListener()
                                    {
                                        public void onClick(DialogInterface dialog, int id)
                                        {
                                            dialog.cancel();
                                        }
                                    });

                    AlertDialog alert = alertDialogBuilder.create();
                    alert.show();

                    return true;
                }
                else
                {
                    /*
                    if (menuId == R.id.action_certstore_add_manual)
                    {
                        Intent intent = new Intent(MissionListActivity.this, MissionEditActivity.class);
                        startActivityForResult(intent, Constants.EDIT_ACTION_REQUEST_CODE);
                        return true;
                    }
                     */
                }

                return false;
            }
        });

        popup.show();
    }

    private void selectFileForImport()
    {
        try
        {
            Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
            intent.setType("*/*");
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            startActivityForResult(Intent.createChooser(intent, getString(R.string.select_a_file)), Constants.PICK_CERTSTORE_FILE_REQUEST_CODE);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    private void importCertStoreFromUri(Uri uri)
    {
        if(uri == null)
        {
            return;
        }

        try
        {
            String displayName;
            String importedFileName;

            displayName = Utils.queryName(getContentResolver(), uri);
            if(Utils.isEmptyString(displayName))
            {
                displayName = uri.getPath();
                displayName = displayName.substring(displayName.lastIndexOf('/') + 1);
            }

            importedFileName = "{" + UUID.randomUUID().toString() + "}-" + displayName;

            File fo = File.createTempFile("import-", "-import", getCacheDir());//NON-NLS
            String fn = fo.getAbsolutePath();
            fo.deleteOnExit();
            BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(fo));

            BufferedInputStream bis = new BufferedInputStream(getContentResolver().openInputStream(uri));
            byte[] data = new byte[8192];
            int numRead;

            while( (numRead = bis.read(data)) > 0 )
            {
                bos.write(data, 0, numRead);
            }

            bis.close();
            bos.close();

            importCertStoreFromFile(fn, importedFileName);

            fo.delete();
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    private void importCertStoreFromFile(String fn, String importedFileName)
    {
        if(Utils.isEmptyString(fn))
        {
            return;
        }

        _importedStore = null;
        boolean autoActivate = false;

        try
        {
            String descriptorText = Globals.getEngageApplication().getEngine().engageQueryCertStoreContents(fn, "");
            if(Utils.isEmptyString(descriptorText))
            {
                Utils.showLongPopupMsg(this, getString(R.string.invalid_cert_store_cannot_obtain_descriptor));
                throw new Exception("");
            }

            JSONObject descriptor = null;

            try
            {
                descriptor = new JSONObject(descriptorText);
                if(descriptor == null)
                {
                    throw new Exception("");
                }
            }
            catch (Exception e)
            {
                Utils.showLongPopupMsg(this, getString(R.string.invalid_cert_store_cannot_decode_descriptor));
                throw e;
            }

            int version = descriptor.optInt(Engine.JsonFields.CertStoreDescriptor.version, 0);
            if(version <= 0)
            {
                Utils.showLongPopupMsg(this, getString(R.string.invalid_cert_store_cannot_invalid_version) + version);
                throw new Exception("");
            }

            String csName = descriptor.optString("name", "");

            JSONArray certificates = null;
            try
            {
                certificates = descriptor.getJSONArray(Engine.JsonFields.CertStoreDescriptor.certificates);
                if(certificates == null)
                {
                    throw new Exception("");
                }

                if(certificates.length() <= 0)
                {
                    throw new Exception("");
                }
            }
            catch (Exception e)
            {
                Utils.showLongPopupMsg(this, getString(R.string.invalid_cert_store_cannot_no_certificates_found));
                throw e;
            }

            try
            {
                // Copy the file into our cache
                String path = Globals.getEngageApplication().getCertStoreCacheDir() + "/" + importedFileName;//NON-NLS
                File fo = new File(path);
                fo.createNewFile();
                BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(fo));
                File fi = new File(fn);
                BufferedInputStream bis = new BufferedInputStream(new FileInputStream(fi));
                byte[] data = new byte[2048];
                int numRead;
                while ((numRead = bis.read(data)) > 0)
                {
                    bos.write(data, 0, numRead);
                }
                bis.close();
                bos.close();

                // Add the certstore to our list of stores
                _importedStore = EngageCertStore.loadStoreFrom(fo.getAbsolutePath());
                if(_importedStore != null)
                {
                    _stores.add(_importedStore);
                    _adapter.notifyDataSetChanged();

                    if(_importedStore.idMatches(_activeMissionCertStoreId))
                    {
                        autoActivate = true;
                    }
                }
            }
            catch (Exception e)
            {
                Utils.showLongPopupMsg(this, getString(R.string.io_error_while_importing));
                throw e;
            }
        }
        catch(Exception e)
        {
            e.printStackTrace();
        }

        if(autoActivate)
        {
            AlertDialog dlg = new AlertDialog.Builder(this)
                    .setTitle(getString(R.string.auto_activate_cert_store_title))
                    .setCancelable(true)
                    .setPositiveButton(R.string.button_ok, new DialogInterface.OnClickListener()
                    {
                        @Override
                        public void onClick(DialogInterface dialogInterface, int i)
                        {
                            activateCertStoreAndFinish(_importedStore);
                        }
                    }).setMessage(getString(R.string.auto_activate_cert_store))
                    .create();

            dlg.show();
        }
    }

    private void activateCertStoreAndFinish(final EngageCertStore cs)
    {
        _activeCertStoreFileName = (cs == null ? "" : cs.getFileName());

        SharedPreferences.Editor ed = Globals.getSharedPreferencesEditor();
        ed.putString(PreferenceKeys.USER_CERT_STORE_FILE_NAME, _activeCertStoreFileName);
        ed.apply();

        _resultIntent.putExtra(Constants.CERTSTORE_CHANGED_TO_FN, _activeCertStoreFileName);
        setResult(RESULT_OK, _resultIntent);
        finish();
    }

    private void viewCertStore(final EngageCertStore cs)
    {
        String message = cs.getPopupDescriptionAsHtml();

        Spanned spannedMessage;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.N)
        {
            spannedMessage = Html.fromHtml(message, Html.FROM_HTML_MODE_LEGACY);
        }
        else
        {
            spannedMessage = Html.fromHtml(message);
        }

        AlertDialog dlg = new AlertDialog.Builder(CertStoreListActivity.this)
                .setTitle(cs.getDisplayName())
                .setMessage(spannedMessage)
                .setCancelable(true)
                .create();

        dlg.show();
    }

    private void confirmDeleteCertStore(final EngageCertStore cs)
    {
        final boolean isActive = (cs.getFileName().compareTo(_activeCertStoreFileName) == 0 || cs.idMatches(_activeMissionCertStoreId));
        String s;

        if(isActive)
        {
            s = getString(R.string.cert_store_in_use_proceed_with_delete_will_use_default);
        }
        else
        {
            s = getString(R.string.are_you_sure_delete_cert_store);
        }

        final TextView message = new TextView(this);
        final SpannableString ss = new SpannableString(s);

        message.setText(ss);
        message.setMovementMethod(LinkMovementMethod.getInstance());
        message.setPadding(32, 32, 32, 32);

        AlertDialog dlg = new AlertDialog.Builder(this)
                .setTitle(getString(R.string.delete_cert_store))
                .setCancelable(false)
                .setPositiveButton(R.string.button_yes, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                        for(EngageCertStore c : _stores)
                        {
                            if(c.getFileName().compareTo(cs.getFileName()) == 0)
                            {
                                try
                                {
                                    File f = new File(cs.getFileName());
                                    f.delete();
                                    _stores.remove(c);
                                    _adapter.notifyDataSetChanged();

                                    if(isActive)
                                    {
                                        activateCertStoreAndFinish(null);
                                    }
                                }
                                catch (Exception e)
                                {
                                    e.printStackTrace();
                                }

                                break;
                            }
                        }
                    }
                }).setNegativeButton(R.string.button_cancel, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i)
                    {
                    }
                }).setView(message).create();

        dlg.show();
    }
}
