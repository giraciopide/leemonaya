package leemonaya

import cats.effect.Sync
import io.circe.Json
import org.http4s.HttpRoutes
import org.http4s.circe.jsonEncoder
import org.http4s.dsl.Http4sDsl

case class StationFeedMsg(stationId: String, temperature: Double, humidity: Double)

class ServerRoutes[F[_]: Sync] extends Http4sDsl[F] {
  val routes: HttpRoutes[F] =
    HttpRoutes.of[F] {
      case GET -> Root / "hello" / name =>
        Ok(Json.obj("message" -> Json.fromString(s"Hello, ${name}")))
    }
}

