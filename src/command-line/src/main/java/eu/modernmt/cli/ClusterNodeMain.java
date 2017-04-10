package eu.modernmt.cli;

import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import eu.modernmt.cli.log4j.Log4jConfiguration;
import eu.modernmt.cluster.ClusterNode;
import eu.modernmt.cluster.EmbeddedService;
import eu.modernmt.config.*;
import eu.modernmt.config.xml.XMLConfigBuilder;
import eu.modernmt.engine.Engine;
import eu.modernmt.facade.ModernMT;
import eu.modernmt.io.DefaultCharset;
import eu.modernmt.rest.RESTServer;
import org.apache.commons.cli.*;
import org.apache.commons.io.FileUtils;

import java.io.File;
import java.io.IOException;
import java.lang.reflect.Field;

/**
 * Created by davide on 22/04/16.
 */
public class ClusterNodeMain {

    private static class Args {

        private static final Options cliOptions;

        static {
            Option engine = Option.builder("e").longOpt("engine").hasArg().required().build();
            Option statusFile = Option.builder().longOpt("status-file").hasArg().required().build();
            Option logsFolder = Option.builder().longOpt("logs").hasArg().required().build();

            Option apiPort = Option.builder("a").longOpt("api-port").hasArg().type(Integer.class).required(false).build();
            Option clusterPort = Option.builder("p").longOpt("cluster-port").hasArg().type(Integer.class).required(false).build();
            Option datastreamPort = Option.builder().longOpt("datastream-port").hasArg().required(false).build();
            Option databasePort = Option.builder().longOpt("db-port").hasArg().required(false).build();

            Option member = Option.builder().longOpt("member").hasArg().required(false).build();

            Option verbosity = Option.builder("v").longOpt("verbosity").hasArg().type(Integer.class).required(false).build();

            cliOptions = new Options();
            cliOptions.addOption(engine);
            cliOptions.addOption(apiPort);
            cliOptions.addOption(clusterPort);
            cliOptions.addOption(statusFile);
            cliOptions.addOption(verbosity);
            cliOptions.addOption(member);
            cliOptions.addOption(logsFolder);
            cliOptions.addOption(datastreamPort);
            cliOptions.addOption(databasePort);
        }

        public final String engine;
        public final File statusFile;
        public final File logsFolder;
        public final int verbosity;
        public final NodeConfig config;

        public Args(String[] args) throws ParseException, ConfigException {
            CommandLineParser parser = new DefaultParser();
            CommandLine cli = parser.parse(cliOptions, args);

            this.engine = cli.getOptionValue("engine");
            this.statusFile = new File(cli.getOptionValue("status-file"));
            this.logsFolder = new File(cli.getOptionValue("logs"));

            String verbosity = cli.getOptionValue("verbosity");
            this.verbosity = verbosity == null ? 1 : Integer.parseInt(verbosity);

            this.config = XMLConfigBuilder.build(Engine.getConfigFile(this.engine));
            this.config.getEngineConfig().setName(this.engine);

            String port = cli.getOptionValue("cluster-port");
            if (port != null) {
                NetworkConfig netConfig = this.config.getNetworkConfig();
                netConfig.setPort(Integer.parseInt(port));
            }

            String apiPort = cli.getOptionValue("api-port");
            if (apiPort != null) {
                ApiConfig apiConfig = this.config.getNetworkConfig().getApiConfig();
                apiConfig.setPort(Integer.parseInt(apiPort));
            }

            String datastreamPort = cli.getOptionValue("datastream-port");
            if (datastreamPort != null)
                this.config.getDataStreamConfig().setPort(Integer.parseInt(datastreamPort));

            String databasePort = cli.getOptionValue("db-port");
            if (databasePort != null)
                this.config.getDatabaseConfig().setPort(Integer.parseInt(databasePort));

            String member = cli.getOptionValue("member");


            if (member != null) {
                String[] parts = member.split(":");

                JoinConfig joinConfig = this.config.getNetworkConfig().getJoinConfig();

                JoinConfig.Member[] members = new JoinConfig.Member[1];
                members[0] = new JoinConfig.Member(parts[0], Integer.parseInt(parts[1]), 0);

                /* If there are members in hazelcast cluster,
                *  update configurations accordingly*/
                joinConfig.setMembers(members);

                if (config.getDataStreamConfig().getType() == DataStreamConfig.Type.EMBEDDED)
                    this.config.getDataStreamConfig().setHost(parts[0]);
                if (config.getDatabaseConfig().getType() == DatabaseConfig.Type.EMBEDDED)
                    this.config.getDatabaseConfig().setHost(parts[0]);
            }
        }
    }

    public static void main(String[] _args) throws Throwable {
        Args args = new Args(_args);
        Log4jConfiguration.setup(args.verbosity, args.logsFolder);

        FileStatusListener listener = new FileStatusListener(args.statusFile, args.config);

        try {
            ModernMT.start(args.config, listener);

            ApiConfig apiConfig = args.config.getNetworkConfig().getApiConfig();

            if (apiConfig.isEnabled()) {
                RESTServer.ServerOptions options = new RESTServer.ServerOptions(apiConfig.getPort());
                options.contextPath = apiConfig.getApiRoot();

                RESTServer restServer = new RESTServer(options);
                restServer.start();
            }

            listener.storeStatus(ClusterNode.Status.READY);
        } catch (Throwable e) {
            listener.onError();
            throw e;
        }
    }

    private static class FileStatusListener implements ClusterNode.StatusListener {

        private final File file;
        private final JsonObject state;

        public FileStatusListener(File file, NodeConfig config) {
            this.file = file;

            NetworkConfig netConfig = config.getNetworkConfig();
            ApiConfig apiConfig = netConfig.getApiConfig();
            DatabaseConfig dbConfig = config.getDatabaseConfig();
            DataStreamConfig streamConfig = config.getDataStreamConfig();

            this.state = new JsonObject();

            if (apiConfig.isEnabled()) {
                JsonObject api = new JsonObject();
                api.addProperty("port", apiConfig.getPort());
                String root = apiConfig.getApiRoot();
                if (root != null)
                    api.addProperty("root", root);
                this.state.add("api", api);
            }

            if (dbConfig.isEnabled()) {
                JsonObject db = new JsonObject();
                db.addProperty("port", dbConfig.getPort());
                db.addProperty("host", dbConfig.getHost());
                this.state.add("database", db);
            }

            if (streamConfig.isEnabled()) {
                JsonObject stream = new JsonObject();
                stream.addProperty("port", streamConfig.getPort());
                stream.addProperty("host", streamConfig.getHost());
                this.state.add("datastream", stream);
            }

            this.state.addProperty("cluster_port", netConfig.getPort());
        }

        @Override
        public void onStatusChanged(ClusterNode node, ClusterNode.Status currentStatus, ClusterNode.Status previousStatus) {
            JsonArray array = new JsonArray();

            for (EmbeddedService service : node.getServices()) {
                for (Process process : service.getSubprocesses()) {
                    int pid = getPid(process);

                    if (pid > 0)
                        array.add(pid);
                }
            }

            this.state.add("embedded_services", array);

            if (currentStatus == ClusterNode.Status.READY)
                return; // Wait for REST Api to be ready

            storeStatus(currentStatus);
        }

        public void storeStatus(ClusterNode.Status status) {
            storeStatus(status.toString());
        }

        public void onError() {
            storeStatus("ERROR");
        }

        private void storeStatus(String status) {
            this.state.addProperty("status", status);

            try {
                FileUtils.write(file, this.state.toString(), DefaultCharset.get(), false);
            } catch (IOException e) {
                // Nothing to do
            }
        }
    }

    private static int getPid(Process process) {
        try {
            Field pid = process.getClass().getDeclaredField("pid");
            pid.setAccessible(true); // allows access to non-public fields
            return pid.getInt(process);
        } catch (Throwable e) {
            return -1;
        }
    }

}