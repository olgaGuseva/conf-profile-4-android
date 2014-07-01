package no.infoss.confprofile.vpn;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Map;

import no.infoss.confprofile.BuildConfig;
import no.infoss.confprofile.util.MiscUtils;
import no.infoss.confprofile.util.PcapOutputStream;
import no.infoss.confprofile.vpn.VpnManagerService.VpnConfigInfo;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

public abstract class VpnTunnel implements Runnable {
	public static final int LOG_VERBOSE = 2;
	public static final int LOG_DEBUG = 3;
	public static final int LOG_INFO = 4;
	public static final int LOG_WARN = 5;
	public static final int LOG_ERROR = 6;
	public static final int LOG_FATAL = 7;
	
	protected final String mInstanceLogTag;
	protected final Logger mLogger;
	protected Thread mThread;
	protected Context mCtx;
	protected VpnConfigInfo mCfg;
	protected long mVpnServiceCtx; //native
	protected long mVpnTunnelCtx; //native
	protected final Handler mHandler = new Handler(Looper.getMainLooper());
	
	public VpnTunnel(Context ctx, VpnConfigInfo cfg) {
		mThread = new Thread(this);
		mCtx = ctx;
		mCfg = cfg;
		
		mInstanceLogTag = String.format("%s (id=%d)", getClass().getSimpleName(), mThread.getId());
		mLogger = new Logger();
	}
	
	protected abstract String getThreadName();
	public abstract void establishConnection(Map<String, Object> options);
	public abstract void terminateConnection();
	
	public final String getTunnelId() {
		if(mCfg == null) {
			return null;
		}
		
		return mCfg.configId;
	}
	
	public final Context getContext() {
		return mCtx;
	}
	
	public void startLoop() {
		mThread.setName(getThreadName());
		mThread.start();
	}
	
	/*package*/ long getTunnelCtx() {
		return mVpnTunnelCtx;
	}
	
	/*package*/ void processDied() {
		//default implementation
	}
	
	protected void setMasqueradeIp4Mode(boolean isOn) {
		setMasqueradeIp4Mode(mVpnTunnelCtx, isOn);
	}
	
	protected void setMasqueradeIp4(int ip4) {
		setMasqueradeIp4(mVpnTunnelCtx, ip4);
	}
	
	protected void setMasqueradeIp6Mode(boolean isOn) {
		setMasqueradeIp6Mode(mVpnTunnelCtx, isOn);
	}
	
	protected void setMasqueradeIp6(byte[] ip6) {
		if(ip6 == null || ip6.length != 16) {
			return;
		}
		setMasqueradeIp6(mVpnTunnelCtx, ip6);
	}
	
	protected void debugRestartPcap(PcapOutputStream pos) {
		debugRestartPcap(mVpnTunnelCtx, pos);
	}
	
	protected void debugStopPcap() {
		debugStopPcap(mVpnTunnelCtx);
	}
	
	private native void setMasqueradeIp4Mode(long vpnTunnelCtx, boolean isOn);
	private native void setMasqueradeIp4(long vpnTunnelCtx, int ip4);
	private native void setMasqueradeIp6Mode(long vpnTunnelCtx, boolean isOn);
	private native void setMasqueradeIp6(long vpnTunnelCtx, byte[] ip6);
	
	private native void debugRestartPcap(long vpnTunnelCtx, PcapOutputStream pos);
	private native void debugStopPcap(long vpnTunnelCtx);
	
	private final void debugLogMessage(int level, String msg) {
		switch (level) {
		case LOG_FATAL: 
		case LOG_ERROR: {
			Log.e(mInstanceLogTag, msg);
			break;
		}
		case LOG_WARN: {
			Log.w(mInstanceLogTag, msg);
			break;
		}
		case LOG_INFO: {
			Log.i(mInstanceLogTag, msg);
			break;
		}
		case LOG_DEBUG: {
			Log.d(mInstanceLogTag, msg);
			break;
		}
		case LOG_VERBOSE: {
			Log.v(mInstanceLogTag, msg);
			break;
		}
		default: {
			//silently drop this message
			break;
		}
		}
	}
	
	/*package*/ class Logger {
		private static final String LOG_FMT = "[%s] [level=%d] %s\n";
		
		private boolean mIsInitialized = false;
		private boolean mIsDeactivated = false;
		private SimpleDateFormat mFmt = new SimpleDateFormat("HH:mm:ss.SSS");
		private File mLogFile;
		private OutputStream mOs;
		
		private synchronized void init() {
			if(mIsInitialized || mIsDeactivated) {
				return;
			}
			
			try {
				if(!MiscUtils.isExternalStorageWriteable()) {
					Log.e(mInstanceLogTag, "Logger is unable to write external storage (storage is RO or unmounted");
					mIsDeactivated = true;
					return;
				}
				
				File externalFilesDir = VpnTunnel.this.getContext().getExternalFilesDir(null);
				if(externalFilesDir == null) {
					//error: storage error
					Log.e(mInstanceLogTag, "Logger is unable to write external storage (storage error)");
					return;
				}
				
				String logFileName = String.format("report-%d-(%s).log", 
						System.currentTimeMillis(), 
						VpnTunnel.this.getTunnelId());
				mLogFile = new File(externalFilesDir, logFileName);
				FileOutputStream fos = new FileOutputStream(mLogFile);
				mOs = new BufferedOutputStream(fos, 128 * 1024);
				
				String headerFmt = "Current date is %s\n";
				SimpleDateFormat dateFmt = new SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss.SSSZ");
				mOs.write(String.format(headerFmt, dateFmt.format(new Date())).getBytes("UTF-8"));
				
				mIsInitialized = true;
			} catch(Exception e) {
				Log.e(mInstanceLogTag, "Can't init logger", e);
				close();
			}
		}
		
		public synchronized void log(int level, String data) {
			if(BuildConfig.DEBUG) {
				VpnTunnel.this.debugLogMessage(level, data);
			}
			
			if(mIsDeactivated) {
				return;
			}
			
			if(!mIsInitialized) {
				init();
			}
			
			if(!mIsInitialized) {
				return;
			}
			
			try {
				synchronized(mFmt) {
					String logMsg = String.format(LOG_FMT, mFmt.format(new Date()), level, data);
					mOs.write(logMsg.getBytes("UTF-8"));
				}
			} catch(Exception e) {
				Log.e(mInstanceLogTag, "Can't log a message, closing a logger", e);
				close();
			}
			
			if(BuildConfig.DEBUG) {
				VpnTunnel.this.debugLogMessage(level, data);
			}
		}
		
		public void close() {
			mIsInitialized = false;
			mIsDeactivated = true;
		}
	}
}
